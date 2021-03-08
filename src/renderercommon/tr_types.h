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
 * @file tr_types.h
 *
 * @warning Never ever change existing members of structs in this file, it'll break 2.60 mod compatibility
 *          when they are referenced in cgame or ui
 */

#ifndef INCLUDE_TR_TYPES_H
#define INCLUDE_TR_TYPES_H

// renderer2 BEGIN
#define MAX_REF_LIGHTS      1024
#define MAX_BONES           128         ///< RB: same as MDX_MAX_BONES
#define MAX_WEIGHTS         4           ///< GPU vertex skinning limit, never change this without rewriting many GLSL shaders
// renderer2 END

#define MAX_CORONAS         64          ///< not really a reason to limit this other than trying to keep a reasonable count
                                        ///< increased from 32 to 64 for ETL
                                        ///< mods compiling against vanilla engine don't use it and/or use the lower value which doesn't affect us
#define MAX_DLIGHTS         32          ///< can't be increased, because bit flags are used on surfaces
#define MAX_ENTITIES        1023        ///< can't be increased without changing drawsurf bit packing - keep this for the mod

// renderfx flags
#define RF_MINLIGHT         0x000001    ///< allways have some light (viewmodel, some items)
#define RF_THIRD_PERSON     0x000002    ///< don't draw through eyes, only mirrors (player bodies, chat sprites)
#define RF_FIRST_PERSON     0x000004    ///< only draw through eyes (view weapon, damage blood blob)
#define RF_DEPTHHACK        0x000008    ///< for view weapon Z crunching
#define RF_NOSHADOW         0x000010    ///< don't add stencil shadows

#define RF_LIGHTING_ORIGIN  0x000020    ///< use refEntity->lightingOrigin instead of refEntity->origin
                                        ///< for lighting.  This allows entities to sink into the floor
                                        ///< with their origin going solid, and allows all parts of a
                                        ///< player to get the same lighting
#define RF_SHADOW_PLANE     0x0100      ///< use refEntity->shadowPlane  0x000040
#define RF_WRAP_FRAMES      0x0200      ///< mod the model frames by the maxframes to allow continuous  0x000080
                                        ///< animation without needing to know the frame count
#define RF_HILIGHT          0x000100    ///< more than RF_MINLIGHT.  For when an object is "Highlighted" (looked at/training identification/etc)
#define RF_BLINK            0x000200    ///< eyes in 'blink' state
#define RF_FORCENOLOD       0x000400

// refdef flags
#define RDF_NOWORLDMODEL    1           ///< used for player configuration screen
#define RDF_HYPERSPACE      4           ///< teleportation effect

#define RDF_SKYBOXPORTAL    8

#define RDF_UNDERWATER      (1 << 4)    ///< so the renderer knows to use underwater fog when the player is underwater
#define RDF_DRAWINGSKY      (1 << 5)
#define RDF_SNOOPERVIEW     (1 << 6)	// night vision device

// renderer2 BEGIN
#define REFENTITYNUM_BITS   10          ///< can't be increased without changing drawsurf bit packing

/// the last N-bit number (2^REFENTITYNUM_BITS - 1) is reserved for the special world refentity,
/// and this is reflected by the value of MAX_REFENTITIES (which therefore is not a power-of-2)
#define MAX_REFENTITIES     ((1 << REFENTITYNUM_BITS) - 1)
// renderer2 END

/**
 * @struct polyVert_t
 * @brief
 */
typedef struct
{
	vec3_t xyz;
	float st[2];
	byte modulate[4];
} polyVert_t;

/**
 * @struct poly_s
 * @typedef poly_t
 * @brief
 */
typedef struct poly_s
{
	qhandle_t hShader;
	int numVerts;
	polyVert_t *verts;
} poly_t;

/**
 * @enum refEntityType_t
 * @brief
 */
typedef enum
{
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_SPLASH,              ///< ripple effect
	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_CORE_TAPER,     ///< a modified core that creates a properly texture mapped core that's wider at one end
	RT_RAIL_RINGS,
	RT_LIGHTNING,
	RT_PORTALSURFACE,       ///< doesn't draw anything, just info for portals

	RT_MAX_REF_ENTITY_TYPE
} refEntityType_t;

#define REFLAG_ONLYHAND     1   ///< only draw hand surfaces
#define REFLAG_FORCE_LOD    8   ///< force a low lod
#define REFLAG_ORIENT_LOD   16  ///< on LOD switch, align the model to the player's camera
#define REFLAG_DEAD_LOD     32  ///< allow the LOD to go lower than recommended

// renderer2 BEGIN

//#define USE_REFLIGHT 1

/// having bone names for each refEntity_t takes several MiBs
/// in backEndData_t so only use it for debugging and development
/// enabling this will show the bone names with r_showSkeleton 1
//#define REFBONE_NAMES 1

#if defined(USE_REFENTITY_ANIMATIONSYSTEM)

/**
 * @struct refBone_t
 * @brief
 */
typedef struct
{
#if defined(REFBONE_NAMES)
	char name[64];
#endif
	short parentIndex;           ///< parent index (-1 if root)
	vec3_t origin;
	quat_t rotation;
} refBone_t;

/**
 * @enum refSkeletonType_t
 * @brief
 */
typedef enum
{
	SK_INVALID,
	SK_RELATIVE,
	SK_ABSOLUTE
} refSkeletonType_t;

/**
 * @struct refSkeleton_t
 * @brief
 */
typedef struct
{
	refSkeletonType_t type;     ///< skeleton has been reset

	short numBones;
	refBone_t bones[MAX_BONES];

	vec3_t bounds[2];           ///< bounds of all applied animations
	vec3_t scale;
} refSkeleton_t;
#endif
// renderer2 END

/**
 * @struct refEntity_t
 * @brief
 */
typedef struct
{
	refEntityType_t reType;
	int renderfx;

	qhandle_t hModel;           ///< opaque type outside refresh

	// most recent data
	vec3_t lightingOrigin;      ///< so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float shadowPlane;          ///< projection shadows go here, stencils go slightly lower

	vec3_t axis[3];             ///< rotation vectors
	vec3_t torsoAxis[3];        ///< rotation vectors for torso section of skeletal animation
	qboolean nonNormalizedAxes; ///< axis are not normalized, i.e. they have scale
	vec3_t origin;              ///< also used as MODEL_BEAM's "from"
	int frame;                  ///< also used as MODEL_BEAM's diameter
	qhandle_t frameModel;
	int torsoFrame;             ///< skeletal torso can have frame independant of legs frame
	qhandle_t torsoFrameModel;

	// previous data for frame interpolation
	vec3_t oldorigin;           ///< also used as MODEL_BEAM's "to"
	int oldframe;
	qhandle_t oldframeModel;
	int oldTorsoFrame;
	qhandle_t oldTorsoFrameModel;
	float backlerp;             ///< 0.0 = current, 1.0 = old
	float torsoBacklerp;

	// texturing
	int skinNum;                ///< inline skin index
	qhandle_t customSkin;       ///< NULL for default skin
	qhandle_t customShader;     ///< use one image for the entire thing

	// misc
	byte shaderRGBA[4];         ///< colors used by rgbgen entity shaders
	float shaderTexCoord[2];    ///< texture coordinates used by tcMod entity modifiers
	float shaderTime;           ///< subtracted from refdef time to control effect start times

	// extra sprite information
	float radius;
	float rotation;

	vec3_t fireRiseDir;

	/// entity fading (gibs, debris, etc)
	int fadeStartTime, fadeEndTime;

	float hilightIntensity;

	int reFlags;

	int entityNum;              ///< currentState.number, so we can attach rendering effects to specific entities (Zombie)

	// renderer2 BEGIN

#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
	/// extra animation information
	refSkeleton_t skeleton;
#endif

#if defined(USE_REFENTITY_NOSHADOWID)
	// extra light interaction information
	short noShadowID;
#endif

	// renderer2  END

} refEntity_t;

// renderer2 BEGIN
/**
 * @enum refLightType_t
 * @brief
 */
typedef enum
{
	RL_OMNI,            ///< point light
	RL_PROJ,            ///< spot light
	RL_DIRECTIONAL,     ///< sun light

	RL_MAX_REF_LIGHT_TYPE
} refLightType_t;

/**
 * @struct refLight_t
 * @brief
 */
typedef struct
{
	refLightType_t rlType;

	qhandle_t attenuationShader;

	vec3_t origin;
	quat_t rotation;
	vec3_t center;
	vec3_t color;               ///< range from 0.0 to 1.0, should be color normalized

	float scale;                ///< r_lightScale if not set

	/// omni-directional light specific
	vec3_t radius;

	// projective light specific
	vec3_t projTarget;
	vec3_t projRight;
	vec3_t projUp;
	vec3_t projStart;
	vec3_t projEnd;

	qboolean noShadows;
	short noShadowID;           ///< don't cast shadows of all entities with this id

	qboolean inverseShadows;    ///< don't cast light and draw shadows by darken the scene
	                            ///< this is useful for drawing player shadows with shadow mapping
} refLight_t;
// renderer2 END

/**
 * @enum glfogType_t
 * @brief
 * @warning synch FOG_SERVER in sv_ccmds.c if you change anything
 */
typedef enum
{
	FOG_NONE,       ///<  0

	FOG_SKY,        ///<  1   fog values to apply to the sky when using density fog for the world (non-distance clipping fog) (only used if(glfogsettings[FOG_MAP].registered) or if(glfogsettings[FOG_MAP].registered))
	FOG_PORTALVIEW, ///<  2   used by the portal sky scene
	FOG_HUD,        ///<  3   used by the 3D hud scene

	//      The result of these for a given frame is copied to the scene.glFog when the scene is rendered

	// the following are fogs applied to the main world scene
	FOG_MAP,        ///<  4   use fog parameter specified using the "fogvars" in the sky shader
	FOG_WATER,      ///<  5   used when underwater
	FOG_SERVER,     ///<  6   the server has set my fog (probably a target_fog) (keep synch in sv_ccmds.c !!!)
	FOG_CURRENT,    ///<  7   stores the current values when a transition starts
	FOG_LAST,       ///<  8   stores the current values when a transition starts
	FOG_TARGET,     ///<  9   the values it's transitioning to.

	FOG_CMD_SWITCHFOG,  ///< 10   transition to the fog specified in the second parameter of R_SetFog(...) (keep synch in sv_ccmds.c !!!)

	NUM_FOGS
} glfogType_t;

/**
 * @struct glfog_t
 * @brief
 */
typedef struct
{
	int mode;                   ///< GL_LINEAR, GL_EXP
	int hint;                   ///< GL_DONT_CARE
	int startTime;              ///< in ms
	int finishTime;             ///< in ms
	float color[4];
	float start;                ///< near
	float end;                  ///< far
	qboolean useEndForClip;     ///< use the 'far' value for the far clipping plane
	float density;              ///< 0.0-1.0
	qboolean registered;        ///< has this fog been set up?
	qboolean drawsky;           ///< draw skybox
	qboolean clearscreen;       ///< clear the GL color buffer
} glfog_t;

#define MAX_RENDER_STRINGS          8
#define MAX_RENDER_STRING_LENGTH    32

/**
 * @struct refdef_t
 * @brief
 */
typedef struct
{
	int x, y, width, height;
	float fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[3];         ///< transformation matrix

	int time;                   ///< time in milliseconds for shader effects and other time dependent rendering issues
	int rdflags;                ///< RDF_NOWORLDMODEL, etc

	/// 1 bits will prevent the associated area from rendering at all
	byte areamask[MAX_MAP_AREA_BYTES];

	/// text messages for deform text shaders
	char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	/// needed to pass fog infos into the portal sky scene
	glfog_t glfog;

} refdef_t;

/**
 * @enum textureCompression_t
 * @brief
 */
typedef enum
{
	TC_NONE,
	TC_S3TC,                ///< this is for the GL_S3_s3tc extension.
	TC_S3TC_ARB             ///< this is for the GL_EXT_texture_compression_s3tc extension.
} textureCompression_t;

/**
 * @enum glDriverType_t
 * @brief
 */
typedef enum
{
	GLDRV_ICD,              ///< driver is integrated with window system
	GLDRV_STANDALONE,       ///< deprecated
	GLDRV_VOODOO,           ///< deprecated
} glDriverType_t;

/**
 * @enum glHardwareType_t
 * @brief
 */
typedef enum
{
	GLHW_GENERIC,           ///< where everthing works the way it should
	GLHW_3DFX_2D3D,         ///< deprecated
	GLHW_RIVA128,           ///< deprecated
	GLHW_RAGEPRO,           ///< deprecated
	GLHW_PERMEDIA2,         ///< deprecated
} glHardwareType_t;

/**
 * @struct glconfig_t
 *
 * @brief Contains variables specific to the OpenGL configuration
 * being run right now.  These are constant once the OpenGL
 * subsystem is initialized.
 */
typedef struct
{
	char renderer_string[MAX_STRING_CHARS];
	char vendor_string[MAX_STRING_CHARS];
	char version_string[MAX_STRING_CHARS];

	char extensions_string[MAX_STRING_CHARS * 4];   ///< bumping, some cards have a big extension string
	                                                ///< - no need to increase MAX_STRING_CHARS *4 - console doesn't print more
	                                                ///< ET:L also stores this data in char* to fix extensions_string overflow issues
	                                                ///< see end of struct - modern gfx cards have huge extensions string

	int maxTextureSize;                             ///< queried from GL
	int maxActiveTextures;                          ///< multitexture ability

	int colorBits, depthBits, stencilBits;

	glDriverType_t driverType;                      ///< obsolete, kept for compatibility
	glHardwareType_t hardwareType;                  ///< obsolete, kept for compatibility

	qboolean deviceSupportsGamma;
	textureCompression_t textureCompression;
	qboolean textureEnvAddAvailable;
	qboolean anisotropicAvailable;
	float maxAnisotropy;

	// vendor-specific support
	qboolean NVFogAvailable;                        ///< obsolete, kept for compatibility
	int NVFogMode;                                  ///< obsolete, kept for compatibility
	int ATIMaxTruformTess;                          ///< obsolete, kept for compatibility
	int ATINormalMode;                              ///< obsolete, kept for compatibility
	int ATIPointMode;                               ///< obsolete, kept for compatibility

	int vidWidth, vidHeight;
	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	// normal screens should be 4/3, but wide aspect monitors may be 16/9
	float windowAspect;
	float displayAspect;

	int displayFrequency;

	// synonymous with "does rendering consume the entire screen?"
	qboolean isFullscreen;
	qboolean smpActive;                     ///< obsolete, kept for compatibility

	// extra general info - keep at end for backward compatibility
	char shadingLanguageVersion[MAX_STRING_CHARS];
	int glslMajorVersion;
	int glslMinorVersion;

	int contextCombined;
} glconfig_t;

/**
 * @enum windowContextType_t
 * @brief
 */
typedef enum
{
	GL_CONTEXT_DEFAULT,
	GL_CONTEXT_COMP,
	GL_CONTEXT_CORE,
	GL_CONTEXT_EGL,
} windowContextType_t;

/**
 * @struct windowContext_s
 * @typedef windowContext_t
 * @brief
 */
typedef struct windowContext_s
{
	int versionMajor;
	int versionMinor;
	int context;
} windowContext_t;

// =========================================
// these MUST NOT exceed the values for SHADER_MAX_VERTEXES/SHADER_MAX_INDEXES
#define MAX_PB_VERTS    1025
#define MAX_PB_INDICIES (MAX_PB_VERTS * 6)

/**
 * @struct polyBuffer_s
 * @typedef polyBuffer_t
 * @brief
 */
typedef struct polyBuffer_s
{
	vec4_t xyz[MAX_PB_VERTS];
	vec2_t st[MAX_PB_VERTS];
	byte color[MAX_PB_VERTS][4];
	int numVerts;

	unsigned int indicies[MAX_PB_INDICIES];
	int numIndicies;

	qhandle_t shader;
} polyBuffer_t;
// =========================================

#endif  // INCLUDE_TR_TYPES_H
