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
 * @file vulkan/vk.c
 * @brief Vulkan backend implementation: instance/device/surface/
 * swapchain, render passes, pipelines, buffers, descriptor sets, the
 * frame loop and the 2D/3D draw submission used by the tr_*.c code.
 */

#include <vulkan/vulkan.h>
#include <string.h>
#include <stdlib.h>

#include "vk.h"
#include "vk_funcs.h"
#include "../../renderercommon/tr_common.h"
#include "../tr_local.h"

// Generated (GLSL -> SPIR-V -> C header) by cmake/ETLBin2H.cmake at build
// time; see the FEATURE_RENDERER_VULKAN block in cmake/ETLBuildRenderer.cmake.
#include "ui.vert.glsl.h"
#include "ui.frag.glsl.h"
#include "tr3d.vert.glsl.h"
#include "tr3d.frag.glsl.h"
#include "tr3d_lm.vert.glsl.h"
#include "tr3d_lm.frag.glsl.h"
#include "tr3d_static.vert.glsl.h"
#include "tr3d_diffuse.vert.glsl.h"
#include "tr3d_skin.vert.glsl.h"
#include "tr3d_md3.vert.glsl.h"
#include "tr3d_lm_static_dlight.vert.glsl.h"
#include "tr3d_lm_dlight.frag.glsl.h"
#include "tr3d_2stage.vert.glsl.h"
#include "tr3d_2stage.frag.glsl.h"
#include "ui_bindless.vert.glsl.h"
#include "ui_bindless.frag.glsl.h"
#include "tr3d_lm_static.vert.glsl.h"
#include "gamma.vert.glsl.h"
#include "gamma.frag.glsl.h"
#include "world_cull.comp.glsl.h"
#include "world_gather.comp.glsl.h"
#include "tr3d_static_bl.vert.glsl.h"
#include "tr3d_lm_static_bl.vert.glsl.h"
#include "tr3d_bl.frag.glsl.h"
#include "tr3d_lm_bl.frag.glsl.h"
#include "tr3d_md3_bl.vert.glsl.h"
#include "tr3d_skin_bl.vert.glsl.h"
#include "tr3d_static_fog_bl.vert.glsl.h"
#include "tr3d_foliage.vert.glsl.h"
#include "tr3d_foliage_fog.vert.glsl.h"

#define VK_MAX_SWAPCHAIN_IMAGES 8

// Frames the CPU may record ahead of the GPU. Each frame slot has its own
// command buffer, fence, acquire semaphore, deferred-free list and region
// of every vertex/index arena (the VK_*_ARENA_SIZE values are per frame).
#define VK_FRAMES_IN_FLIGHT 2

// GPU world culling: world views per frame that can be culled on the GPU
// (the main view, a portal or mirror, the sky portal); further views
// fall back to the CPU world walk
#define VK_CULL_MAX_VIEWS 4

// merged world batches (VK_BatchWorldDraw): pending keys and draws before a
// flush; per frame at most VK_CULL_MAX_VIEWS * groups * MAX_SHADER_STAGES
// draws (vk.cullMaxBatchDraws)
#define VK_WB_MAX_KEYS  128
#define VK_WB_MAX_DRAWS 2048

// merged model batches (VK_BatchModelDraw): pending keys and draws before a
// flush, draws per frame slot
#define VK_MB_MAX_KEYS    64
#define VK_MB_MAX_PENDING 1024
#define VK_MB_SLOT_DRAWS  4096

// per-draw data of a merged model batch (tr3d_md3_bl.vert's ModelDraw)
typedef struct
{
	float mvp[16];
	float modelView[16];
	float alphaTest[4];        // y: tcGen environment
	float lightDir[4];         // lightDir, ambient, directed, lerp: push constant slots 48-63
	float ambient[4];
	float directed[4];
	float lerp[4];
	float env[12];             // RB_VkSetupEnvParams
	uint32_t ofs[4];           // mesh buffer word offsets: frame, oldframe, texcoords, compressed frame
	uint32_t ofs2[4];          // compressed oldframe, bindless texture slot
} vkModelDraw_t;

// per-draw data of a merged world batch (tr3d_*static_bl.vert's WorldDraw)
typedef struct
{
	uint32_t textures[4];      // base, lightmap bindless slots
	float colorMul[4];
	float colorAdd[4];
	float fogS[4];             // fog pass (VK_Draw3DStaticFog): RB_CalcFogTexCoordParams
	float fogT[4];
	float fogEye[4];
} vkWorldDraw_t;

// world_cull.comp push constants (128 bytes)
typedef struct
{
	float frustum[5][4];       // normal, dist
	float viewOrigin[4];
	unsigned int numSurfs;
	unsigned int leafBitsOfs;  // word offsets in the cull arena
	unsigned int skipOfs;
	unsigned int cmdOfs;
	unsigned int viewStamp;
	unsigned int flags;        // VK_CULL_NOCULL, VK_CULL_FACEPLANE
	unsigned int pad[2];
} vkCullPush_t;

// A world can have many distinct shader stateBits/cullType combinations;
// pipelines are created lazily and cached for the life of the renderer.
#define VK_MAX_SCENE_PIPELINES 256

// Non-GLS pipeline states used by stencil shadow volumes
// (renderer1's RB_ShadowTessEnd()/RB_ShadowFinish()).
#define VK_SPECIAL_NONE           0
#define VK_SPECIAL_SHADOW_INCR    1 // volume faces: no color, stencil ALWAYS, pass -> increment
#define VK_SPECIAL_SHADOW_DECR    2 // volume faces: no color, stencil ALWAYS, pass -> decrement
#define VK_SPECIAL_SHADOW_DARKEN  3 // full-screen darken where stencil != 0
// Line-list topology for debug drawing (immediate-mode
// GL_LINES emulation, r_showTris, r_showNormals, bone/bounds debug)
#define VK_SPECIAL_LINES          4

// Where a scene pipeline reads its vertices from: the per-frame arena
// (vk3DVertex_t / vk3DVertexLM_t written per draw) or the per-map static
// world buffer (vk3DVertexLM_t), whose single-texture pipelines bind either
// the texture or the lightmap coords as their UV.
#define VK_VERTS_ARENA            0
#define VK_VERTS_STATIC           1 // static buffer; uv = texture coords (lightmapped: uv0 texture, uv1 lightmap)
#define VK_VERTS_STATIC_LIGHTMAP  2 // static buffer, single texture; uv = lightmap coords
#define VK_VERTS_DIFFUSE          3 // arena, single texture, vk3DVertexN_t: rgb from entity lighting (tr3d_diffuse.vert)
#define VK_VERTS_SKIN             4 // skin buffer (vkSkinVertex_t), MDM skinned in tr3d_skin.vert, bones in set 1
#define VK_VERTS_STATIC_DLIGHT    5 // static buffer, lightmapped, dlight pass folded in (dlights in set 2)
#define VK_VERTS_TWOSTAGE         6 // arena, vk3DVertex2_t: two stages in one draw (tr3d_2stage.*), 2 texture sets
#define VK_VERTS_STATIC_BL        8 // as VK_VERTS_STATIC, merged world batch: bindless textures, per-draw data (tr3d_*static_bl.vert)
#define VK_VERTS_STATIC_LIGHTMAP_BL 9 // as VK_VERTS_STATIC_LIGHTMAP, merged world batch
#define VK_VERTS_STATIC_FOG_BL    12 // merged world batch, fog pass of a group: fog texcoords from per-draw parameters (tr3d_static_fog_bl.vert)
#define VK_VERTS_SKIN_BL          11 // as VK_VERTS_SKIN, merged model batch: bones from the bone arena as storage (tr3d_skin_bl.vert)
#define VK_VERTS_MD3_BL           10 // as VK_VERTS_MD3, merged model batch: no vertex streams (tr3d_md3_bl.vert reads the mesh buffer)
#define VK_VERTS_FOLIAGE          13 // static buffer + per-instance origin/color stream (foliageInstance_t), tr3d_foliage.vert
#define VK_VERTS_FOLIAGE_FOG      14 // as VK_VERTS_FOLIAGE, fog pass: fog texcoords from push constants (tr3d_foliage_fog.vert)
#define VK_VERTS_MD3              7 // mesh buffer: frame, oldframe (md3XyzNormal_t), texcoords, MDC compressed frame, oldframe; tr3d_md3.vert, normal tables in set 1

// MD3/MDC frames + texcoords + triangles of every loaded model (VK_WriteMeshData)
#define VK_MESH_BUFFER_SIZE       (32 * 1024 * 1024)

// bindless texture array size (slots, see vk.bindless): MAX_DRAWIMAGES plus
// room for images recreated while their old slot waits for its frame fence
#define VK_BINDLESS_MAX_IMAGES    4096

// GPU skinning (VK_VERTS_SKIN): the skin vertex buffer holds every loaded MDM
// surface (reset with the models), the bone arena one surface's bones per
// GPU-skinned batch (3 vec4 rows each, read as a dynamic uniform buffer)
#define VK_SKIN_BUFFER_SIZE       (16 * 1024 * 1024)
#define VK_SKIN_MAX_BONES         256
#define VK_SKIN_BONE_BYTES        (3 * 4 * sizeof(float))
#define VK_SKIN_BONE_ARENA_SIZE   (2 * 1024 * 1024)
// range of vk.boneSet's dynamic uniform buffer: VK_SKIN_MAX_BONES bones or
// the mesh normal tables (VK_VERTS_MD3), whichever is larger
#define VK_UNIFORM_RANGE          (FUNCTABLE_SIZE * sizeof(float))

// mesh normal tables (VK_VERTS_MD3, set 1): the tr.sinTable entries an MD3/MDC
// normal can read (lat/long * 16, or one less for an interpolated MD3 normal)
// as VK_MESH_SIN_PAIRS pairs (sin[16 * k], sin[16 * k - 1]), then r_anormals
// as vec4s
#define VK_MESH_SIN_PAIRS         (FUNCTABLE_SIZE / 16)
#define VK_MESH_TABLE_FLOATS      (2 * VK_MESH_SIN_PAIRS + 4 * NUMMDCVERTEXNORMALS)

// Scene push constants: mat4 mvp, mat4 modelView, vec4 alphaTest, vec4
// fogColor, vec4 fogParams, vec4 clipPlane (48 floats, every scene draw),
// then vec4 colorMul, vec4 colorAdd (static world draws) or vec4 lightDir,
// vec4 ambient, vec4 directed (VK_VERTS_DIFFUSE/SKIN/MD3 draws), then vec4
// frame interpolation (VK_VERTS_MD3). GPU-skinned/animated draws use
// alphaTest.y (the texEnv mode, unused there) as their tcGen environment flag
// and ambient.w as the VK_MESH_* flags.
#define VK_SCENE_PUSH_FLOATS      64

// One lazily-created pipeline variant for a given (stateBits, cullType,
// polygonOffset) combination used by the 3D scene pipelines.
typedef struct
{
	unsigned long stateBits;
	int cullType;
	qboolean polygonOffset;
	int special; // VK_SPECIAL_* (stencil shadow pipelines), 0 = normal
	int vertexSource; // VK_VERTS_*
	VkPipeline pipeline;
} vkScenePipelineEntry_t;

// One lazily-created pipeline variant for a given stateBits value used by
// the 2D/UI pipeline (see VK_GetUIPipeline()). UI shader stages (e.g. the
// automap circular-mask trick in cg_commandmap.c, which relies on
// alphafunc + depthFunc equal) need their real stateBits honored instead
// of always forcing the same fixed blend-only state.
typedef struct
{
	unsigned long stateBits;
	VkPipeline pipeline;
} vkUIPipelineEntry_t;

typedef struct
{
	VkInstance instance;
	uint32_t instanceVersion;                              // VK_API_VERSION_1_0 or _1_2 (see VK_CreateInstance)
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	uint32_t graphicsQueueFamily;
	uint32_t presentQueueFamily;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapchain;
	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;
	uint32_t swapchainImageCount;
	VkImage swapchainImages[VK_MAX_SWAPCHAIN_IMAGES];
	VkImageView swapchainImageViews[VK_MAX_SWAPCHAIN_IMAGES];
	VkFramebuffer swapchainFramebuffers[VK_MAX_SWAPCHAIN_IMAGES]; // presentRenderPass targets

	// The main render pass (vk.renderPass) draws the whole
	// frame into this offscreen color image; VK_EndFrame() then copies it
	// to the swapchain through the gamma pass (renderer1's mainFbo +
	// R_ScreenGamma()).
	VkImage sceneColorImage;

	// Render resolution of the scene targets (color, MSAA, depth) - the
	// window size times r_scale (glConfig.vidWidth/vidHeight). The gamma
	// pass scales the scene image to the swapchain (renderer1's mainFbo
	// blit to the window). sceneSize is the requested size, 0 = swapchain.
	VkExtent2D sceneExtent;
	VkExtent2D sceneSize;

	// MSAA (r_ext_multisample). With sampleCount > 1 the main
	// render pass draws into msColorImage + a multisampled depth buffer
	// and resolves into sceneColorImage (renderer1's msMainFbo blit).
	VkSampleCountFlagBits sampleCount;
	VkImage msColorImage;
	VkDeviceMemory msColorMemory;
	VkImageView msColorView;
	// the persistent color target(s) were just (re)created and hold no
	// defined contents yet: VK_BeginFrame() clears them once
	qboolean sceneColorNeedsInit;
	VkDeviceMemory sceneColorMemory;
	VkImageView sceneColorView;
	VkFramebuffer sceneFramebuffer;
	VkRenderPass presentRenderPass;
	VkDescriptorSet sceneColorDescriptor;
	VkShaderModule gammaVertModule;
	VkShaderModule gammaFragModule;
	VkPipelineLayout gammaPipelineLayout;
	VkPipeline gammaPipeline;
	float gammaValue;
	float overbrightScale;

	VkRenderPass renderPass;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;                         // current frame slot's (alias)
	VkCommandBuffer commandBuffers[VK_FRAMES_IN_FLIGHT];
	int frameIndex;                                        // current frame slot

	// Depth buffer, recreated alongside the swapchain.
	VkFormat depthFormat;
	qboolean depthHasStencil;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkSemaphore imageAvailableSemaphore;                   // current aliases, set in VK_BeginFrame()
	VkSemaphore renderFinishedSemaphore;
	VkFence inFlightFence;
	VkSemaphore imageAvailableSemaphores[VK_FRAMES_IN_FLIGHT];
	VkSemaphore renderFinishedSemaphores[VK_MAX_SWAPCHAIN_IMAGES]; // per swapchain image
	VkFence inFlightFences[VK_FRAMES_IN_FLIGHT];

	// UI/2D rendering pipeline.
	VkDescriptorSetLayout uiDescriptorSetLayout;
	VkPipelineLayout uiPipelineLayout;
	vkUIPipelineEntry_t uiPipelines[VK_MAX_SCENE_PIPELINES];
	int numUIPipelines;
	VkDescriptorPool uiDescriptorPool;
	VkSampler uiSamplerRepeat; // non-mipmapped images (UI, lightmaps, ...): linear, level 0 only
	VkSampler uiSamplerClamp;

	// Samplers for mipmapped images, built from r_textureMode
	// (GL_TextureMode) and r_ext_texture_filter_anisotropic.
	VkSampler mipSamplerRepeat;
	VkSampler mipSamplerClamp;
	int texFilterMin;            // GL enum, e.g. GL_LINEAR_MIPMAP_NEAREST
	int texFilterMag;            // GL enum, GL_NEAREST or GL_LINEAR
	float maxSamplerAnisotropy;  // 0 when samplerAnisotropy is unsupported

	// Single-frame-in-flight "arena" vertex/index buffers for 2D draws,
	// persistently mapped and reset at the start of every frame. Safe
	// because there is only one command buffer/fence in flight (see
	// VK_BeginFrame/VK_EndFrame) so the CPU never writes into memory the
	// GPU might still be reading.
	VkBuffer vertexArenaBuffer;
	VkDeviceMemory vertexArenaMemory;
	void *vertexArenaMapped;
	VkDeviceSize vertexArenaSize;
	VkDeviceSize vertexArenaOffset;

	VkBuffer indexArenaBuffer;
	VkDeviceMemory indexArenaMemory;
	void *indexArenaMapped;
	VkDeviceSize indexArenaSize;
	VkDeviceSize indexArenaOffset;

	// Static 3D geometry pipelines + resources.
	VkPipelineLayout scenePipelineLayout;   // 1 descriptor set (base texture)
	VkPipelineLayout sceneLMPipelineLayout; // 2 descriptor sets (base + lightmap)

	VkShaderModule sceneVertModule;
	VkShaderModule sceneFragModule;
	VkShaderModule sceneLMVertModule;
	VkShaderModule sceneLMFragModule;
	VkShaderModule sceneStaticVertModule;   // static world buffer variants (color = vertex * mul + add)
	VkShaderModule sceneDiffuseVertModule;  // rgbGen lightingDiffuse variants (VK_VERTS_DIFFUSE)
	VkShaderModule sceneLMStaticVertModule;
	VkShaderModule sceneFoliageVertModule;    // instanced foliage (VK_VERTS_FOLIAGE)
	VkShaderModule sceneFoliageFogVertModule; // its fog pass (VK_VERTS_FOLIAGE_FOG)

	VkShaderModule uiVertModule;
	VkShaderModule uiFragModule;

	vkScenePipelineEntry_t scenePipelines[VK_MAX_SCENE_PIPELINES];
	int numScenePipelines;
	vkScenePipelineEntry_t sceneLMPipelines[VK_MAX_SCENE_PIPELINES];
	int numSceneLMPipelines;

	// Shared across UI and scene draws: tracks whichever pipeline (UI or
	// one of the scene variants) was bound most recently in this command
	// buffer, so a redundant vkCmdBindPipeline can be skipped safely even
	// when UI and 3D draws are interleaved within the same frame (e.g. the
	// limbo menu's 2D background/buttons around its small 3D model preview).
	VkPipeline lastBoundPipeline;

	// Per-command-buffer bind caches (reset in VK_BeginFrame): skip
	// redundant descriptor-set binds, push only changed push-constant
	// ranges, keep the arena buffers bound (draws address them with
	// firstIndex/vertexOffset), and set the 2D viewport only when a 3D
	// view or depth-range change touched it.
	VkPipelineLayout boundSetLayout;
	VkDescriptorSet boundSets[2];
	uint32_t boundSetCount;
	VkPipelineLayout pushLayout;
	float pushData[VK_SCENE_PUSH_FLOATS];
	uint32_t pushBytes;
	VkBuffer boundVertexBuffer;
	VkBuffer boundIndexBuffer;
	qboolean viewport2DSet;

	// GL-convention (column-major) projection/modelview matrices captured
	// where renderer1 calls glLoadMatrixf() in tr_backend.c (see
	// VK_SetProjectionMatrix/VK_SetModelViewMatrix). projMatrix already has
	// the GL->Vulkan clip-space correction (Y-flip + depth 0..1) baked in.
	float projMatrix[16];
	float modelMatrix[16];
	float mvpMatrix[16];

	// Current fog state, mirroring renderer1's glfogsettings
	// (R_Fog()/R_FogOff() in tr_main.c call VK_SetFog()/VK_SetFogOff()
	// where renderer1 calls glFog*()). Linear (start/end) or exponential
	// (GL_EXP, density) fog.
	qboolean fogEnabled;
	qboolean fogExp;         // GL_EXP density fog instead of GL_LINEAR
	float fogDensity;        // GL_FOG_DENSITY for fogExp
	qboolean fogParamsValid; // VK_SetFog() has run at least once
	float clearColor[4];     // Last glClearColor() value

	// Image resources replaced while the current frame's
	// command buffer may still reference them (cinematic scratch image
	// resize); freed at the next VK_BeginFrame() after the fence wait.
	struct
	{
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkDescriptorSet descriptorSet;
		int bindlessSlot;                                  // slot + 1, 0 = none
	} pendingFree[VK_FRAMES_IN_FLIGHT][8];                 // per frame slot
	int numPendingFree[VK_FRAMES_IN_FLIGHT];

	// Frame capture for screenshots / levelshots / avi
	// frames. When requested, VK_EndFrame() copies the final (post-gamma)
	// swapchain image into captureBuffer and waits for it.
	VkBuffer captureBuffer;
	VkDeviceMemory captureMemory;
	VkDeviceSize captureSize;
	void *captureMapped;
	qboolean captureRequested;
	qboolean captureReady;
	uint32_t captureWidth;
	uint32_t captureHeight;
	VkFormat captureFormat;
	float fogColor[4];
	float fogStart;
	float fogEnd;

	// Last scene viewport rect (post Y-flip), stashed so
	// VK_SetDepthRange() can reissue vkCmdSetViewport with the same
	// rect but a different depth range (sky is drawn pinned to the far
	// depth plane, same as renderer1's glDepthRange(1,1) trick).
	int lastViewportX;
	int lastViewportFlippedY;
	int lastViewportWidth;
	int lastViewportHeight;

	// Portal/mirror eye-space clip plane, mirroring
	// renderer1's glClipPlane(GL_CLIP_PLANE0, ...)/glEnable(GL_CLIP_PLANE0)
	// (RB_BeginDrawingView in tr_backend.c). Vulkan has no
	// fixed-function clip-plane equivalent that's portable without extra
	// device features, so this is applied as a per-fragment discard in
	// the scene shaders instead (see tr3d*.frag.glsl).
	qboolean clipPlaneEnabled;
	float clipPlane[4];

	VkBuffer sceneVertexArenaBuffer;
	VkDeviceMemory sceneVertexArenaMemory;
	void *sceneVertexArenaMapped;
	VkDeviceSize sceneVertexArenaSize;
	VkDeviceSize sceneVertexArenaOffset;

	VkBuffer sceneIndexArenaBuffer;
	VkDeviceMemory sceneIndexArenaMemory;
	void *sceneIndexArenaMapped;
	VkDeviceSize sceneIndexArenaSize;
	VkDeviceSize sceneIndexArenaOffset;

	// per-map static world vertex buffer (vk3DVertexLM_t: xyz, texture
	// coords, lightmap coords, raw BSP vertex color), see R_LoadStaticWorldVertices()
	VkBuffer staticWorldBuffer;
	VkDeviceMemory staticWorldMemory;
	void *staticWorldMapped;
	int staticWorldNumVertexes;

	// GPU world culling (world_cull.comp, VK_CreateWorldCull/VK_CullWorldView):
	// static per-map data (surfaces, their leaves, their indexes into the
	// static world buffer), the host-written per-frame arena (per frame
	// slot: skip stamps, then per view the visible-leaf bitset and one
	// indirect draw command per group) and the output index buffer (per
	// frame slot and view, every group's region)
	VkBuffer cullSurfBuffer, cullLeafBuffer, cullIndexBuffer, cullArenaBuffer, cullOutBuffer;
	VkDeviceMemory cullSurfMemory, cullLeafMemory, cullIndexMemory, cullArenaMemory, cullOutMemory;
	unsigned int *cullArenaMapped;
	VkDescriptorSetLayout cullSetLayout;
	VkDescriptorPool cullPool;
	VkDescriptorSet cullSet;
	VkPipelineLayout cullPipelineLayout;
	VkPipeline cullPipeline;
	VkCommandBuffer cullCommandBuffers[VK_FRAMES_IN_FLIGHT];
	qboolean cullRecording;                                // this frame's cull command buffer is begun
	qboolean inFrame;                                      // between VK_BeginFrame and VK_EndFrame
	int cullNumViews;                                      // views culled this frame
	int cullNumSurfs, cullNumGroups, cullNumIndexes;
	unsigned int cullLeafWords, cullViewWords, cullSlotWords;
	unsigned int cullStamp;                                // last view stamp (never 0)

	// merged world batches (VK_BatchWorldDraw/VK_FlushWorldBatch): GPU-culled
	// group draws with the same pipeline and view state become one
	// multi-draw indirect call. The gather pass (world_gather.comp, end of
	// the frame's cull command buffer) copies each draw's group command to
	// its slot in cullBatchCmdBuffer; vkWorldDraw_t per slot in
	// cullWorldDrawBuffer (bindless texture slots, stage colors)
	qboolean multiDrawIndirect;                            // multiDrawIndirect + drawIndirectFirstInstance enabled
	VkBuffer cullGatherBuffer, cullBatchCmdBuffer, cullWorldDrawBuffer;
	VkDeviceMemory cullGatherMemory, cullBatchCmdMemory, cullWorldDrawMemory;
	unsigned int *cullGatherMapped;
	vkWorldDraw_t *cullWorldDrawMapped;
	int cullMaxBatchDraws;                                 // per frame slot
	VkPipeline gatherPipeline;
	VkDescriptorSetLayout worldDrawSetLayout;
	VkDescriptorPool worldDrawPool;
	VkDescriptorSet worldDrawSet;
	VkPipelineLayout sceneBindlessPipelineLayout;          // set 0 bindless textures, set 1 per-draw data
	VkShaderModule sceneStaticBLVertModule, sceneLMStaticBLVertModule, sceneBLFragModule, sceneLMBLFragModule, sceneStaticFogBLVertModule;
	int wbFrameDraws;                                      // slots used this frame
	struct
	{
		VkPipeline pipeline;
		float header[48];                                  // push constants up to the stage colors
	} wbKeys[VK_WB_MAX_KEYS];
	int wbNumKeys;
	struct
	{
		int key;
		unsigned int srcWord;                              // group command in the cull arena
		vkWorldDraw_t data;
	} wbDraws[VK_WB_MAX_DRAWS];
	int wbNumDraws;
	unsigned int wbGroupOffset;                            // group of the last batched draw (its stages chain)
	int wbMinKey;                                          // key of its last stage: later stages go to this or later keys
	qboolean wbFlushing;

	// merged model batches (VK_BatchModelDraw/VK_FlushModelBatch): GPU-animated
	// MD3/MDC draws with the same pipeline and view state become one
	// multi-draw indirect call with CPU-written commands; tr3d_md3_bl.vert
	// reads the frames from the mesh buffer (storage) at per-draw offsets
	qboolean modelBatch;                                   // resources created (VK_CreateModelBatchResources)
	VkDescriptorSetLayout modelSetLayout;
	VkDescriptorPool modelPool;
	VkDescriptorSet modelSet;                              // per-draw data, mesh buffer, bone arena
	VkPipelineLayout sceneMD3BLPipelineLayout;             // set 0 bindless textures, 1 modelSet, 2 mesh normal tables
	VkShaderModule sceneMD3BLVertModule, sceneSkinBLVertModule;
	VkBuffer modelDrawBuffer, modelCmdBuffer;
	VkDeviceMemory modelDrawMemory, modelCmdMemory;
	vkModelDraw_t *modelDrawMapped;
	unsigned int *modelCmdMapped;
	int mbFrameDraws;
	struct
	{
		VkPipeline pipeline;
		VkBuffer vertexBuffer, indexBuffer;
		float header[48];                                  // view state push constants (matrices, env flag zeroed)
	} mbKeys[VK_MB_MAX_KEYS];
	int mbNumKeys;
	struct
	{
		int key;
		unsigned int numIndexes, firstIndex;
		vkModelDraw_t data;
	} mbDraws[VK_MB_MAX_PENDING];
	int mbNumDraws;
	unsigned int mbSurfaceSerial;                          // batch (tess.vkSurfaceSerial) of the last draw
	int mbMinKey;                                          // key of its last stage
	qboolean mbFlushing;

	// GPU skinning (VK_VERTS_SKIN), see VK_WriteSkinVertices/VK_AllocSkinBones
	VkBuffer skinBuffer;
	VkDeviceMemory skinMemory;
	void *skinMapped;
	int skinNumVertexes;
	int skinMaxVertexes;
	VkBuffer boneArenaBuffer;
	VkDeviceMemory boneArenaMemory;
	void *boneArenaMapped;
	VkDeviceSize boneArenaOffset;
	VkDeviceSize boneArenaSize;                            // end of this frame slot's region
	VkDeviceSize uboAlignment;                             // minUniformBufferOffsetAlignment
	VkDescriptorSetLayout boneSetLayout;
	VkDescriptorPool bonePool;
	VkDescriptorSet boneSet;
	VkPipelineLayout skinPipelineLayout;                   // set 0 texture, set 1 bones (or the mesh normal tables), set 2 env params
	VkShaderModule sceneSkinVertModule;

	// GPU MD3 animation (VK_VERTS_MD3), see VK_WriteMeshData/VK_Draw3DMesh
	VkBuffer meshBuffer;
	VkDeviceMemory meshMemory;
	void *meshMapped;
	int meshBytes;
	VkShaderModule sceneMD3VertModule;
	unsigned int meshTablesOffset;                         // this frame's mesh normal tables in the uniform arena
	int meshTablesFrame;                                   // frame they were uploaded for, -1 = none
	int frameCount;                                        // VK_BeginFrame counter
	// static lightmapped batches with the dlight pass folded in
	// (VK_VERTS_STATIC_DLIGHT): set 0 base, 1 lightmap, 2 dlights (boneSet)
	VkPipelineLayout staticDlightPipelineLayout;

	// Bindless textures, used by the 2D path: every image also
	// has a slot in one big combined-image-sampler array (vk.bindlessSet),
	// so 2D draws with different textures but the same state can be batched
	// into one draw (VK_Draw2D / VK_Flush2D). qfalse when the device lacks
	// descriptor indexing or r_vkBindless is 0: per-image sets, no batching.
	qboolean bindless;
	VkDescriptorSetLayout bindlessSetLayout;
	VkDescriptorPool bindlessPool;
	VkDescriptorSet bindlessSet;
	VkPipelineLayout uiBindlessPipelineLayout;
	VkShaderModule uiBindlessVertModule;
	VkShaderModule uiBindlessFragModule;
	int bindlessFree[VK_BINDLESS_MAX_IMAGES];              // free slot stack
	int numBindlessFree;

	// pending 2D batch (bindless only): consecutive VK_Draw2D calls with the
	// same stateBits, drawn as one by VK_Flush2D()
	qboolean batch2DOpen;
	unsigned long batch2DStateBits;
	uint32_t batch2DFirstIndex;
	uint32_t batch2DNumIndexes;
	int32_t batch2DVertexOffset;
	int batch2DNumVertexes;
	VkShaderModule sceneLMStaticDlightVertModule;
	VkShaderModule sceneLMDlightFragModule;
	VkShaderModule scene2StageVertModule;                  // VK_VERTS_TWOSTAGE
	VkShaderModule scene2StageFragModule;

	qboolean initialized;
	uint32_t currentImageIndex;
} vkContext_t;

static vkContext_t vk;

// Vertex format fed into the UI pipeline: screen-space pixel
// position, texture UV, and final (already colorGen/alphaGen-baked) RGBA8
// vertex color.
typedef struct
{
	float pos[2];
	float uv[2];
	byte color[4];
	uint32_t texture; // bindless slot (vk.bindless); unused with per-image sets
} vkUIVertex_t;

// Sized generously: a single 2D shader batch can be up to SHADER_MAX_VERTEXES
// (10000) vertices, and a busy UI frame (console/loading text, HUD, menus)
// can issue dozens of such batches.
#define VK_UI_VERTEX_ARENA_SIZE (16 * 1024 * 1024)
#define VK_UI_INDEX_ARENA_SIZE  (4 * 1024 * 1024)

// Vertex formats fed into the 3D scene pipelines.
typedef struct
{
	float pos[3];
	float uv[2];
	byte color[4];
} vk3DVertex_t;

typedef struct
{
	float pos[3];
	float uv0[2];
	float uv1[2];
	byte color[4];
} vk3DVertexLM_t;

// VK_VERTS_TWOSTAGE: both stages' texture coords and colors
typedef struct
{
	float pos[3];
	float uv0[2];
	float uv1[2];
	byte color0[4];
	byte color1[4];
} vk3DVertex2_t;

// VK_VERTS_DIFFUSE: vk3DVertex_t plus the model-space normal the vertex
// shader lights with (color only supplies alpha)
typedef struct
{
	float pos[3];
	float uv[2];
	byte color[4];
	float normal[3];
} vk3DVertexN_t;

#define VK_SCENE_VERTEX_ARENA_SIZE (32 * 1024 * 1024)
#define VK_SCENE_INDEX_ARENA_SIZE  (8 * 1024 * 1024)
#define VK_UI_MAX_DESCRIPTOR_SETS 2048

#ifdef VK_NO_PROTOTYPES
/*
 * Runtime-loaded Vulkan entry points (Linux/Windows). The renderer only uses
 * the Vulkan headers; every function is fetched from the system loader
 * (libvulkan.so.1 / vulkan-1.dll, opened by SDL) so no Vulkan SDK or import
 * library is needed. The pointers are named like the prototypes they
 * replace, so the rest of this file calls them unchanged.
 */
static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

#define VK_DEFINE_FUNC(name) static PFN_ ## name name;
VK_GLOBAL_FUNCS(VK_DEFINE_FUNC)
VK_INSTANCE_FUNCS(VK_DEFINE_FUNC)
VK_DEVICE_FUNCS(VK_DEFINE_FUNC)
#undef VK_DEFINE_FUNC

/**
 * @brief Resolves the loader-level functions from the vkGetInstanceProcAddr
 * the engine got from the system Vulkan loader.
 */
static qboolean VK_LoadGlobalFunctions(void)
{
	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)ri.VKimp_GetInstanceProcAddr();
	if (!vkGetInstanceProcAddr)
	{
		Ren_Print("Vulkan: could not load the Vulkan library (no vkGetInstanceProcAddr)\n");
		return qfalse;
	}

#define VK_LOAD_FUNC(name) \
		name = (PFN_ ## name)vkGetInstanceProcAddr(NULL, #name); \
		if (!name) { Ren_Print("Vulkan: failed to load %s\n", #name); return qfalse; }
	VK_GLOBAL_FUNCS(VK_LOAD_FUNC)
#undef VK_LOAD_FUNC

	return qtrue;
}

static qboolean VK_LoadInstanceFunctions(void)
{
#define VK_LOAD_FUNC(name) \
		name = (PFN_ ## name)vkGetInstanceProcAddr(vk.instance, #name); \
		if (!name) { Ren_Print("Vulkan: failed to load %s\n", #name); return qfalse; }
	VK_INSTANCE_FUNCS(VK_LOAD_FUNC)
#undef VK_LOAD_FUNC

	return qtrue;
}

/**
 * @brief Device functions are fetched with vkGetDeviceProcAddr, which
 * returns the driver's entry points directly (no loader dispatch).
 */
static qboolean VK_LoadDeviceFunctions(void)
{
#define VK_LOAD_FUNC(name) \
		name = (PFN_ ## name)vkGetDeviceProcAddr(vk.device, #name); \
		if (!name) { Ren_Print("Vulkan: failed to load %s\n", #name); return qfalse; }
	VK_DEVICE_FUNCS(VK_LOAD_FUNC)
#undef VK_LOAD_FUNC

	return qtrue;
}

static void VK_ClearFunctions(void)
{
#define VK_CLEAR_FUNC(name) name = NULL;
	VK_GLOBAL_FUNCS(VK_CLEAR_FUNC)
	VK_INSTANCE_FUNCS(VK_CLEAR_FUNC)
	VK_DEVICE_FUNCS(VK_CLEAR_FUNC)
#undef VK_CLEAR_FUNC
	vkGetInstanceProcAddr = NULL;
}
#else
// macOS: linked against the Vulkan SDK loader, prototypes are real functions
#define VK_LoadGlobalFunctions()   qtrue
#define VK_LoadInstanceFunctions() qtrue
#define VK_LoadDeviceFunctions()   qtrue
#define VK_ClearFunctions()
#endif

/**
 * @brief Minimal error helper: prints the VkResult and returns qfalse so
 * callers can bail out cleanly instead of crashing on an invalid handle.
 */
static qboolean VK_CheckResult(VkResult result, const char *what)
{
	if (result != VK_SUCCESS)
	{
		Ren_Print("Vulkan: %s failed (VkResult %d)\n", what, (int)result);
		return qfalse;
	}
	return qtrue;
}

/**
 * @brief Returns qtrue if the named instance extension is present in the
 * loader's enumeration (used to conditionally opt in to
 * VK_KHR_portability_enumeration, required by MoltenVK-based loaders on
 * macOS since Vulkan SDK 1.3.216 to avoid VK_ERROR_INCOMPATIBLE_DRIVER).
 */
static qboolean VK_HasInstanceExtension(const char *name)
{
	uint32_t              count = 0, i;
	VkExtensionProperties *props;

	if (vkEnumerateInstanceExtensionProperties(NULL, &count, NULL) != VK_SUCCESS || count == 0)
	{
		return qfalse;
	}

	props = malloc(count * sizeof(VkExtensionProperties));
	if (!props)
	{
		return qfalse;
	}

	if (vkEnumerateInstanceExtensionProperties(NULL, &count, props) != VK_SUCCESS)
	{
		free(props);
		return qfalse;
	}

	for (i = 0; i < count; i++)
	{
		if (!strcmp(props[i].extensionName, name))
		{
			free(props);
			return qtrue;
		}
	}

	free(props);
	return qfalse;
}

static qboolean VK_CreateInstance(void)
{
	unsigned int         extCount = 0;
	const char           *extNames[32];
	qboolean             usePortability;
	VkApplicationInfo    appInfo;
	VkInstanceCreateInfo createInfo;

	if (!ri.VKimp_GetInstanceExtensions(&extCount, NULL))
	{
		return qfalse;
	}
	// Reserve one extra slot for VK_KHR_portability_enumeration.
	if (extCount >= (sizeof(extNames) / sizeof(extNames[0])) - 1)
	{
		Ren_Print("Vulkan: too many required instance extensions (%u)\n", extCount);
		return qfalse;
	}
	if (!ri.VKimp_GetInstanceExtensions(&extCount, extNames))
	{
		return qfalse;
	}

	// macOS/MoltenVK loaders expose the ICD as a "portability subset"
	// driver; without requesting this extension and flag, vkCreateInstance
	// fails with VK_ERROR_INCOMPATIBLE_DRIVER on recent SDKs.
	usePortability = VK_HasInstanceExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	if (usePortability)
	{
		extNames[extCount++] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	}

	Com_Memset(&appInfo, 0, sizeof(appInfo));
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = "ET: Legacy";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = "ET: Legacy renderer_vk";
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	// Vulkan 1.2 where the loader supports it (bindless textures need its
	// descriptor indexing); vkEnumerateInstanceVersion is missing on 1.0 loaders
	{
		PFN_vkEnumerateInstanceVersion enumerateVersion = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion");
		uint32_t                       version          = VK_API_VERSION_1_0;

		if (enumerateVersion && enumerateVersion(&version) == VK_SUCCESS && version >= VK_API_VERSION_1_2)
		{
			vk.instanceVersion = VK_API_VERSION_1_2;
		}
		else
		{
			vk.instanceVersion = VK_API_VERSION_1_0;
		}
	}
	appInfo.apiVersion = vk.instanceVersion;

	Com_Memset(&createInfo, 0, sizeof(createInfo));
	createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo        = &appInfo;
	createInfo.enabledExtensionCount   = extCount;
	createInfo.ppEnabledExtensionNames = extNames;
	// NOTE: VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR is a
	// VkInstanceCreateFlagBits enumerator, not a preprocessor macro, so it
	// cannot be probed with #ifdef - it's unconditionally available once
	// vulkan_core.h declares VK_KHR_portability_enumeration (SDK >= 1.3.216).
	if (usePortability)
	{
		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}

	return VK_CheckResult(vkCreateInstance(&createInfo, NULL, &vk.instance), "vkCreateInstance");
}

static qboolean VK_PickPhysicalDeviceAndQueues(void)
{
	uint32_t         deviceCount = 0, i, q;
	VkPhysicalDevice devices[16];
	VkBool32         presentSupport;

	vkEnumeratePhysicalDevices(vk.instance, &deviceCount, NULL);
	if (deviceCount == 0)
	{
		Ren_Print("Vulkan: no physical devices found\n");
		return qfalse;
	}
	if (deviceCount > 16)
	{
		deviceCount = 16;
	}
	vkEnumeratePhysicalDevices(vk.instance, &deviceCount, devices);

	// pick the first device that has both a graphics and a present queue
	for (i = 0; i < deviceCount; i++)
	{
		uint32_t                queueFamilyCount = 0;
		VkQueueFamilyProperties queueFamilies[32];
		int                     graphicsFamily = -1, presentFamily = -1;

		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, NULL);
		if (queueFamilyCount > 32)
		{
			queueFamilyCount = 32;
		}
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies);

		for (q = 0; q < queueFamilyCount; q++)
		{
			if (queueFamilies[q].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				graphicsFamily = (int)q;
			}

			presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], q, vk.surface, &presentSupport);
			if (presentSupport)
			{
				presentFamily = (int)q;
			}
		}

		if (graphicsFamily >= 0 && presentFamily >= 0)
		{
			vk.physicalDevice      = devices[i];
			vk.graphicsQueueFamily = (uint32_t)graphicsFamily;
			vk.presentQueueFamily  = (uint32_t)presentFamily;
			return qtrue;
		}
	}

	Ren_Print("Vulkan: no suitable physical device (graphics+present queue) found\n");
	return qfalse;
}

static qboolean VK_CreateLogicalDevice(void)
{
	float                            queuePriority = 1.0f;
	VkDeviceQueueCreateInfo          queueCreateInfos[2];
	uint32_t                         queueCreateInfoCount = 0;
	VkPhysicalDeviceFeatures         deviceFeatures;
	VkPhysicalDeviceVulkan12Features features12;
	VkDeviceCreateInfo               createInfo;
	const char                       *deviceExtensions[2] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, NULL };
	uint32_t                         deviceExtensionCount = 1;
	uint32_t                         availableExtCount = 0, i;
	VkExtensionProperties            *availableExt;

	Com_Memset(queueCreateInfos, 0, sizeof(queueCreateInfos));

	queueCreateInfos[queueCreateInfoCount].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[queueCreateInfoCount].queueFamilyIndex = vk.graphicsQueueFamily;
	queueCreateInfos[queueCreateInfoCount].queueCount       = 1;
	queueCreateInfos[queueCreateInfoCount].pQueuePriorities = &queuePriority;
	queueCreateInfoCount++;

	if (vk.presentQueueFamily != vk.graphicsQueueFamily)
	{
		queueCreateInfos[queueCreateInfoCount].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[queueCreateInfoCount].queueFamilyIndex = vk.presentQueueFamily;
		queueCreateInfos[queueCreateInfoCount].queueCount       = 1;
		queueCreateInfos[queueCreateInfoCount].pQueuePriorities = &queuePriority;
		queueCreateInfoCount++;
	}

	Com_Memset(&deviceFeatures, 0, sizeof(deviceFeatures));

	// Anisotropic filtering (r_ext_texture_filter_anisotropic)
	{
		VkPhysicalDeviceFeatures   supported;
		VkPhysicalDeviceProperties props;

		vkGetPhysicalDeviceFeatures(vk.physicalDevice, &supported);
		vkGetPhysicalDeviceProperties(vk.physicalDevice, &props);

		vk.maxSamplerAnisotropy = 0.0f;
		if (supported.samplerAnisotropy)
		{
			deviceFeatures.samplerAnisotropy = VK_TRUE;
			vk.maxSamplerAnisotropy          = props.limits.maxSamplerAnisotropy;
		}

		// merged world batches: several draws per indirect call, each with
		// its per-draw data index in firstInstance
		vk.multiDrawIndirect = (supported.multiDrawIndirect && supported.drawIndirectFirstInstance) ? qtrue : qfalse;
		if (vk.multiDrawIndirect)
		{
			deviceFeatures.multiDrawIndirect         = VK_TRUE;
			deviceFeatures.drawIndirectFirstInstance = VK_TRUE;
		}

		// Highest supported sample count <= r_ext_multisample
		{
			VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts &
			                            props.limits.framebufferDepthSampleCounts &
			                            props.limits.framebufferStencilSampleCounts;
			int requested = r_ext_multisample ? r_ext_multisample->integer : 0;
			int samples;

			vk.sampleCount = VK_SAMPLE_COUNT_1_BIT;
			for (samples = 64; samples > 1; samples >>= 1)
			{
				if (samples <= requested && (counts & (VkSampleCountFlags)samples))
				{
					vk.sampleCount = (VkSampleCountFlagBits)samples;
					break;
				}
			}
			if (requested > 1)
			{
				Ren_Print("Vulkan: MSAA %dx (r_ext_multisample %d)\n", (int)vk.sampleCount, requested);
			}
		}
	}

	// MoltenVK-backed physical devices report VK_KHR_portability_subset as
	// a supported device extension; the Vulkan spec requires enabling it
	// at device-creation time whenever the physical device supports it.
	if (vkEnumerateDeviceExtensionProperties(vk.physicalDevice, NULL, &availableExtCount, NULL) == VK_SUCCESS &&
	    availableExtCount > 0)
	{
		availableExt = malloc(availableExtCount * sizeof(VkExtensionProperties));
		if (availableExt && vkEnumerateDeviceExtensionProperties(vk.physicalDevice, NULL, &availableExtCount, availableExt) == VK_SUCCESS)
		{
			for (i = 0; i < availableExtCount; i++)
			{
				if (!strcmp(availableExt[i].extensionName, "VK_KHR_portability_subset"))
				{
					deviceExtensions[deviceExtensionCount++] = "VK_KHR_portability_subset";
					break;
				}
			}
		}
		if (availableExt)
		{
			free(availableExt);
		}
	}

	// Bindless textures (vk.bindless): Vulkan 1.2 descriptor indexing - a
	// partially bound, update-after-bind array of combined image samplers
	// indexed non-uniformly from the fragment shader
	Com_Memset(&features12, 0, sizeof(features12));
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vk.bindless      = qfalse;
	if (r_vkBindless->integer && vk.instanceVersion >= VK_API_VERSION_1_2)
	{
		PFN_vkGetPhysicalDeviceFeatures2   getFeatures2   = (PFN_vkGetPhysicalDeviceFeatures2)vkGetInstanceProcAddr(vk.instance, "vkGetPhysicalDeviceFeatures2");
		PFN_vkGetPhysicalDeviceProperties2 getProperties2 = (PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr(vk.instance, "vkGetPhysicalDeviceProperties2");
		VkPhysicalDeviceProperties         props;

		vkGetPhysicalDeviceProperties(vk.physicalDevice, &props);
		if (getFeatures2 && getProperties2 && props.apiVersion >= VK_API_VERSION_1_2)
		{
			VkPhysicalDeviceVulkan12Features   supported12;
			VkPhysicalDeviceFeatures2          features2;
			VkPhysicalDeviceVulkan12Properties props12;
			VkPhysicalDeviceProperties2        props2;

			Com_Memset(&supported12, 0, sizeof(supported12));
			supported12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
			Com_Memset(&features2, 0, sizeof(features2));
			features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			features2.pNext = &supported12;
			getFeatures2(vk.physicalDevice, &features2);

			Com_Memset(&props12, 0, sizeof(props12));
			props12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
			Com_Memset(&props2, 0, sizeof(props2));
			props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			props2.pNext = &props12;
			getProperties2(vk.physicalDevice, &props2);

			if (supported12.descriptorIndexing && supported12.shaderSampledImageArrayNonUniformIndexing &&
			    supported12.descriptorBindingPartiallyBound && supported12.descriptorBindingSampledImageUpdateAfterBind &&
			    supported12.descriptorBindingUpdateUnusedWhilePending &&
			    props12.maxPerStageDescriptorUpdateAfterBindSampledImages >= VK_BINDLESS_MAX_IMAGES &&
			    props12.maxDescriptorSetUpdateAfterBindSampledImages >= VK_BINDLESS_MAX_IMAGES)
			{
				features12.descriptorIndexing                           = VK_TRUE;
				features12.shaderSampledImageArrayNonUniformIndexing    = VK_TRUE;
				features12.descriptorBindingPartiallyBound              = VK_TRUE;
				features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
				features12.descriptorBindingUpdateUnusedWhilePending    = VK_TRUE;
				vk.bindless                                             = qtrue;
			}
		}
	}
	Ren_Print("Vulkan: bindless textures %s\n", vk.bindless ? "on (2D batching)" : "off");

	Com_Memset(&createInfo, 0, sizeof(createInfo));
	createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext                   = vk.bindless ? &features12 : NULL;
	createInfo.pQueueCreateInfos       = queueCreateInfos;
	createInfo.queueCreateInfoCount    = queueCreateInfoCount;
	createInfo.pEnabledFeatures        = &deviceFeatures;
	createInfo.enabledExtensionCount   = deviceExtensionCount;
	createInfo.ppEnabledExtensionNames = deviceExtensions;

	if (!VK_CheckResult(vkCreateDevice(vk.physicalDevice, &createInfo, NULL, &vk.device), "vkCreateDevice") ||
	    !VK_LoadDeviceFunctions())
	{
		return qfalse;
	}

	vkGetDeviceQueue(vk.device, vk.graphicsQueueFamily, 0, &vk.graphicsQueue);
	vkGetDeviceQueue(vk.device, vk.presentQueueFamily, 0, &vk.presentQueue);

	return qtrue;
}

static qboolean VK_CreateSwapchainAndViews(int width, int height)
{
	VkSurfaceCapabilitiesKHR caps;
	VkSurfaceFormatKHR       formats[32];
	uint32_t                 formatCount = 32;
	VkPresentModeKHR         presentMode = VK_PRESENT_MODE_FIFO_KHR; // vsynced, always supported
	VkExtent2D               extent;
	VkSwapchainCreateInfoKHR createInfo;
	uint32_t                 i;
	uint32_t                 queueFamilyIndices[2];

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physicalDevice, vk.surface, &caps);

	// r_swapInterval (renderer1: SDL_GL_SetSwapInterval): 0 = don't wait
	// for vblank - prefer IMMEDIATE, then MAILBOX; otherwise (or if
	// neither is available) FIFO, which is vsynced and always supported.
	if (ri.Cvar_VariableIntegerValue("r_swapInterval") == 0)
	{
		VkPresentModeKHR modes[16];
		uint32_t         modeCount = 16, m;
		qboolean         haveImmediate = qfalse, haveMailbox = qfalse;

		vkGetPhysicalDeviceSurfacePresentModesKHR(vk.physicalDevice, vk.surface, &modeCount, modes);
		for (m = 0; m < modeCount; m++)
		{
			haveImmediate |= (modes[m] == VK_PRESENT_MODE_IMMEDIATE_KHR);
			haveMailbox   |= (modes[m] == VK_PRESENT_MODE_MAILBOX_KHR);
		}
		presentMode = haveImmediate ? VK_PRESENT_MODE_IMMEDIATE_KHR : (haveMailbox ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR);
	}
	Ren_Developer("Vulkan: present mode %s\n", presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR ? "IMMEDIATE" :
	              presentMode == VK_PRESENT_MODE_MAILBOX_KHR ? "MAILBOX" : "FIFO (vsync)");
	vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physicalDevice, vk.surface, &formatCount, formats);
	if (formatCount == 0)
	{
		Ren_Print("Vulkan: surface has no formats\n");
		return qfalse;
	}

	vk.swapchainFormat = formats[0].format;
	for (i = 0; i < formatCount; i++)
	{
		if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM || formats[i].format == VK_FORMAT_R8G8B8A8_UNORM)
		{
			vk.swapchainFormat = formats[i].format;
			break;
		}
	}

	if (caps.currentExtent.width != 0xFFFFFFFFu)
	{
		extent = caps.currentExtent;
	}
	else
	{
		extent.width  = (uint32_t)Com_Clamp(caps.minImageExtent.width, caps.maxImageExtent.width, (float)width);
		extent.height = (uint32_t)Com_Clamp(caps.minImageExtent.height, caps.maxImageExtent.height, (float)height);
	}
	vk.swapchainExtent = extent;

	vk.swapchainImageCount = caps.minImageCount + 1;
	if (caps.maxImageCount > 0 && vk.swapchainImageCount > caps.maxImageCount)
	{
		vk.swapchainImageCount = caps.maxImageCount;
	}
	if (vk.swapchainImageCount > VK_MAX_SWAPCHAIN_IMAGES)
	{
		vk.swapchainImageCount = VK_MAX_SWAPCHAIN_IMAGES;
	}

	Com_Memset(&createInfo, 0, sizeof(createInfo));
	createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface          = vk.surface;
	createInfo.minImageCount    = vk.swapchainImageCount;
	createInfo.imageFormat      = vk.swapchainFormat;
	createInfo.imageColorSpace  = formats[0].colorSpace;
	createInfo.imageExtent      = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	// Frame capture copies from the swapchain image
	if (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
	{
		createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	queueFamilyIndices[0] = vk.graphicsQueueFamily;
	queueFamilyIndices[1] = vk.presentQueueFamily;
	if (vk.graphicsQueueFamily != vk.presentQueueFamily)
	{
		createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices   = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform   = caps.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode    = presentMode;
	createInfo.clipped        = VK_TRUE;

	if (!VK_CheckResult(vkCreateSwapchainKHR(vk.device, &createInfo, NULL, &vk.swapchain), "vkCreateSwapchainKHR"))
	{
		return qfalse;
	}

	vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &vk.swapchainImageCount, NULL);
	if (vk.swapchainImageCount > VK_MAX_SWAPCHAIN_IMAGES)
	{
		vk.swapchainImageCount = VK_MAX_SWAPCHAIN_IMAGES;
	}
	vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &vk.swapchainImageCount, vk.swapchainImages);

	for (i = 0; i < vk.swapchainImageCount; i++)
	{
		VkImageViewCreateInfo viewInfo;

		Com_Memset(&viewInfo, 0, sizeof(viewInfo));
		viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image                       = vk.swapchainImages[i];
		viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format                      = vk.swapchainFormat;
		viewInfo.components.r                = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g                = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b                = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a                = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;

		if (!VK_CheckResult(vkCreateImageView(vk.device, &viewInfo, NULL, &vk.swapchainImageViews[i]), "vkCreateImageView"))
		{
			return qfalse;
		}
	}

	return qtrue;
}

/**
 * @brief Finds a device memory type index matching both the given
 * memory-type bitmask (from a VkMemoryRequirements) and the requested
 * property flags (host-visible, device-local, ...).
 */
static uint32_t VK_FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
static VkShaderModule VK_CreateShaderModule(const unsigned char *code, unsigned int size);
static void VK_FlushPendingFrees(void);
static void VK_Flush2D(void);
static void VK_Flush2DBatch(void);
static void VK_FlushWorldBatch(void);
static void VK_FlushModelBatch(void);
static void VK_CreateModelBatchResources(void);

/**
 * @brief Creates the depth buffer (image + memory + view)
 * sized to match the scene targets (vk.sceneExtent). Recreated alongside
 * the swapchain on resize (see VK_ResizeSwapchain).
 */
static qboolean VK_CreateDepthResources(void)
{
	VkImageCreateInfo     imageInfo;
	VkMemoryRequirements  memRequirements;
	VkMemoryAllocateInfo  allocInfo;
	VkImageViewCreateInfo viewInfo;

	// Prefer a depth format with stencil (needed by stencil
	// shadow volumes, r_shadows 2). Apple GPUs expose D32_SFLOAT_S8_UINT
	// only; some others D24_UNORM_S8_UINT. Fall back to depth-only.
	{
		static const VkFormat candidates[] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
		VkFormatProperties    props;
		int                   i;

		vk.depthFormat     = VK_FORMAT_D32_SFLOAT;
		vk.depthHasStencil = qfalse;

		for (i = 0; i < (int)ARRAY_LEN(candidates); i++)
		{
			vkGetPhysicalDeviceFormatProperties(vk.physicalDevice, candidates[i], &props);
			if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				vk.depthFormat     = candidates[i];
				vk.depthHasStencil = qtrue;
				break;
			}
		}
	}

	Com_Memset(&imageInfo, 0, sizeof(imageInfo));
	imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width  = vk.sceneExtent.width;
	imageInfo.extent.height = vk.sceneExtent.height;
	imageInfo.extent.depth  = 1;
	imageInfo.mipLevels     = 1;
	imageInfo.arrayLayers   = 1;
	imageInfo.format        = vk.depthFormat;
	imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.samples       = vk.sampleCount;
	imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

	if (!VK_CheckResult(vkCreateImage(vk.device, &imageInfo, NULL, &vk.depthImage), "vkCreateImage(depth)"))
	{
		return qfalse;
	}

	vkGetImageMemoryRequirements(vk.device, vk.depthImage, &memRequirements);

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = VK_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (!VK_CheckResult(vkAllocateMemory(vk.device, &allocInfo, NULL, &vk.depthImageMemory), "vkAllocateMemory(depth)"))
	{
		return qfalse;
	}

	vkBindImageMemory(vk.device, vk.depthImage, vk.depthImageMemory, 0);

	Com_Memset(&viewInfo, 0, sizeof(viewInfo));
	viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image                       = vk.depthImage;
	viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format                      = vk.depthFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (vk.depthHasStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;

	if (!VK_CheckResult(vkCreateImageView(vk.device, &viewInfo, NULL, &vk.depthImageView), "vkCreateImageView(depth)"))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Offscreen color image the main render pass draws
 * into (same format as the swapchain so all existing pipelines stay
 * render-pass compatible; render resolution vk.sceneExtent). Sampled by
 * the gamma pass; TRANSFER_SRC for future screenshot readback.
 */
static qboolean VK_CreateSceneColorResources(void)
{
	VkImageCreateInfo     imageInfo;
	VkMemoryRequirements  memRequirements;
	VkMemoryAllocateInfo  allocInfo;
	VkImageViewCreateInfo viewInfo;

	Com_Memset(&imageInfo, 0, sizeof(imageInfo));
	imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width  = vk.sceneExtent.width;
	imageInfo.extent.height = vk.sceneExtent.height;
	imageInfo.extent.depth  = 1;
	imageInfo.mipLevels     = 1;
	imageInfo.arrayLayers   = 1;
	imageInfo.format        = vk.swapchainFormat;
	imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
	                          VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageInfo.samples     = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (!VK_CheckResult(vkCreateImage(vk.device, &imageInfo, NULL, &vk.sceneColorImage), "vkCreateImage(scene color)"))
	{
		return qfalse;
	}

	vkGetImageMemoryRequirements(vk.device, vk.sceneColorImage, &memRequirements);

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = VK_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (!VK_CheckResult(vkAllocateMemory(vk.device, &allocInfo, NULL, &vk.sceneColorMemory), "vkAllocateMemory(scene color)"))
	{
		return qfalse;
	}

	vkBindImageMemory(vk.device, vk.sceneColorImage, vk.sceneColorMemory, 0);

	Com_Memset(&viewInfo, 0, sizeof(viewInfo));
	viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image                       = vk.sceneColorImage;
	viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format                      = vk.swapchainFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;

	if (!VK_CheckResult(vkCreateImageView(vk.device, &viewInfo, NULL, &vk.sceneColorView), "vkCreateImageView(scene color)"))
	{
		return qfalse;
	}

	// Multisampled color target, resolved into sceneColorImage
	if (vk.sampleCount != VK_SAMPLE_COUNT_1_BIT)
	{
		// persistent like the scene image (no TRANSIENT): its contents
		// carry over between frames
		imageInfo.usage   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageInfo.samples = vk.sampleCount;

		if (!VK_CheckResult(vkCreateImage(vk.device, &imageInfo, NULL, &vk.msColorImage), "vkCreateImage(msaa color)"))
		{
			return qfalse;
		}

		vkGetImageMemoryRequirements(vk.device, vk.msColorImage, &memRequirements);
		allocInfo.allocationSize  = memRequirements.size;
		allocInfo.memoryTypeIndex = VK_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (!VK_CheckResult(vkAllocateMemory(vk.device, &allocInfo, NULL, &vk.msColorMemory), "vkAllocateMemory(msaa color)"))
		{
			return qfalse;
		}
		vkBindImageMemory(vk.device, vk.msColorImage, vk.msColorMemory, 0);

		viewInfo.image = vk.msColorImage;
		if (!VK_CheckResult(vkCreateImageView(vk.device, &viewInfo, NULL, &vk.msColorView), "vkCreateImageView(msaa color)"))
		{
			return qfalse;
		}
	}

	vk.sceneColorNeedsInit = qtrue;

	return qtrue;
}

static qboolean VK_CreateRenderPassAndFramebuffers(void)
{
	VkAttachmentDescription colorAttachment;
	VkAttachmentReference   colorAttachmentRef;
	VkAttachmentDescription depthAttachment;
	VkAttachmentReference   depthAttachmentRef;
	VkAttachmentDescription attachments[3];
	VkAttachmentReference   resolveAttachmentRef;
	VkSubpassDescription    subpass;
	VkSubpassDependency     dependencies[2];
	VkRenderPassCreateInfo  renderPassInfo;
	VkFramebufferCreateInfo framebufferInfo;
	VkImageView             fbAttachments[3];
	uint32_t                i;
	qboolean                msaa = (vk.sampleCount != VK_SAMPLE_COUNT_1_BIT) ? qtrue : qfalse;

	vk.sceneExtent = vk.sceneSize.width ? vk.sceneSize : vk.swapchainExtent;

	if (!VK_CreateDepthResources() || !VK_CreateSceneColorResources())
	{
		return qfalse;
	}

	// --- main (scene) render pass: offscreen color + depth/stencil.
	// The color target keeps its contents between frames (loadOp LOAD),
	// like renderer1's main framebuffer: the frame is never cleared as a
	// whole, only on request (VK_ClearSceneColor - r_clear, r_fastsky,
	// fog clearscreen, ...). Loading screens rely on this: cgame redraws
	// only part of the screen per update and expects the rest to remain.
	Com_Memset(&colorAttachment, 0, sizeof(colorAttachment));
	colorAttachment.format         = vk.swapchainFormat;
	colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // left there by the previous frame
	colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	Com_Memset(&colorAttachmentRef, 0, sizeof(colorAttachmentRef));
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Com_Memset(&depthAttachment, 0, sizeof(depthAttachment));
	depthAttachment.format         = vk.depthFormat;
	depthAttachment.samples        = vk.sampleCount;
	depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp  = vk.depthHasStencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	Com_Memset(&depthAttachmentRef, 0, sizeof(depthAttachmentRef));
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachments[0] = colorAttachment;
	attachments[1] = depthAttachment;

	Com_Memset(&subpass, 0, sizeof(subpass));
	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount    = 1;
	subpass.pColorAttachments       = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	// With MSAA, attachment 0 is the multisampled color
	// target (persistent: loaded and stored, it is what carries the frame
	// over) and attachment 2 the single-sample scene image it resolves
	// into every frame.
	if (msaa)
	{
		attachments[0].samples       = vk.sampleCount;
		attachments[0].storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		attachments[2]               = colorAttachment;
		attachments[2].loadOp        = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[2].storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[2].finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		Com_Memset(&resolveAttachmentRef, 0, sizeof(resolveAttachmentRef));
		resolveAttachmentRef.attachment = 2;
		resolveAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		subpass.pResolveAttachments     = &resolveAttachmentRef;
	}

	Com_Memset(dependencies, 0, sizeof(dependencies));
	// previous frame's gamma pass read the scene image -> this frame writes it
	dependencies[0].srcSubpass   = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass   = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	// previous frame (possibly still executing - VK_FRAMES_IN_FLIGHT > 1):
	// its gamma-pass read and color/depth writes must complete first
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
	                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
	                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	// scene color writes -> gamma pass samples it
	dependencies[1].srcSubpass    = 0;
	dependencies[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	Com_Memset(&renderPassInfo, 0, sizeof(renderPassInfo));
	renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = msaa ? 3 : 2;
	renderPassInfo.pAttachments    = attachments;
	renderPassInfo.subpassCount    = 1;
	renderPassInfo.pSubpasses      = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies   = dependencies;

	if (!VK_CheckResult(vkCreateRenderPass(vk.device, &renderPassInfo, NULL, &vk.renderPass), "vkCreateRenderPass"))
	{
		return qfalse;
	}

	fbAttachments[0] = msaa ? vk.msColorView : vk.sceneColorView;
	fbAttachments[1] = vk.depthImageView;
	fbAttachments[2] = vk.sceneColorView;

	Com_Memset(&framebufferInfo, 0, sizeof(framebufferInfo));
	framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass      = vk.renderPass;
	framebufferInfo.attachmentCount = msaa ? 3 : 2;
	framebufferInfo.pAttachments    = fbAttachments;
	framebufferInfo.width           = vk.sceneExtent.width;
	framebufferInfo.height          = vk.sceneExtent.height;
	framebufferInfo.layers          = 1;

	if (!VK_CheckResult(vkCreateFramebuffer(vk.device, &framebufferInfo, NULL, &vk.sceneFramebuffer), "vkCreateFramebuffer(scene)"))
	{
		return qfalse;
	}

	// --- present render pass: swapchain color only, fully overwritten by
	// the gamma pass
	colorAttachment.loadOp        = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	subpass.pDepthStencilAttachment = NULL;
	subpass.pResolveAttachments     = NULL;
	colorAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;

	Com_Memset(dependencies, 0, sizeof(dependencies));
	dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass    = 0;
	dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments    = &colorAttachment;
	renderPassInfo.dependencyCount = 1;

	if (!VK_CheckResult(vkCreateRenderPass(vk.device, &renderPassInfo, NULL, &vk.presentRenderPass), "vkCreateRenderPass(present)"))
	{
		return qfalse;
	}

	for (i = 0; i < vk.swapchainImageCount; i++)
	{
		fbAttachments[0] = vk.swapchainImageViews[i];

		framebufferInfo.renderPass      = vk.presentRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments    = fbAttachments;
		framebufferInfo.width           = vk.swapchainExtent.width;
		framebufferInfo.height          = vk.swapchainExtent.height;

		if (!VK_CheckResult(vkCreateFramebuffer(vk.device, &framebufferInfo, NULL, &vk.swapchainFramebuffers[i]), "vkCreateFramebuffer"))
		{
			return qfalse;
		}
	}

	return qtrue;
}

/**
 * @brief Gamma pass shader modules + pipeline layout (one
 * combined image sampler, gamma/overbright push constants). Created once.
 */
static qboolean VK_CreateGammaPipelineLayout(void)
{
	VkPushConstantRange        pushRange;
	VkPipelineLayoutCreateInfo layoutInfo;

	vk.gammaVertModule = VK_CreateShaderModule(gamma_vert_spv, gamma_vert_spv_SIZE);
	vk.gammaFragModule = VK_CreateShaderModule(gamma_frag_spv, gamma_frag_spv_SIZE);
	if (vk.gammaVertModule == VK_NULL_HANDLE || vk.gammaFragModule == VK_NULL_HANDLE)
	{
		return qfalse;
	}

	Com_Memset(&pushRange, 0, sizeof(pushRange));
	pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushRange.size       = 2 * sizeof(float);

	Com_Memset(&layoutInfo, 0, sizeof(layoutInfo));
	layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount         = 1;
	layoutInfo.pSetLayouts            = &vk.uiDescriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges    = &pushRange;

	vk.gammaValue      = 1.0f;
	vk.overbrightScale = 1.0f;

	return VK_CheckResult(vkCreatePipelineLayout(vk.device, &layoutInfo, NULL, &vk.gammaPipelineLayout), "vkCreatePipelineLayout(gamma)");
}

/**
 * @brief Per-swapchain present resources - the gamma pipeline
 * (bound to presentRenderPass) and the descriptor set sampling the scene
 * color image. Recreated with the swapchain.
 */
static qboolean VK_CreatePresentResources(void)
{
	VkPipelineShaderStageCreateInfo        stages[2];
	VkPipelineVertexInputStateCreateInfo   vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkPipelineViewportStateCreateInfo      viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo   multisampling;
	VkPipelineColorBlendAttachmentState    colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo    colorBlending;
	VkDynamicState                         dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo       dynamicState;
	VkGraphicsPipelineCreateInfo           pipelineInfo;
	VkDescriptorSetAllocateInfo            dsAllocInfo;
	VkDescriptorImageInfo                  imageDescInfo;
	VkWriteDescriptorSet                   writeSet;

	Com_Memset(stages, 0, sizeof(stages));
	stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vk.gammaVertModule;
	stages[0].pName  = "main";
	stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = vk.gammaFragModule;
	stages[1].pName  = "main";

	Com_Memset(&vertexInputInfo, 0, sizeof(vertexInputInfo));
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	Com_Memset(&inputAssembly, 0, sizeof(inputAssembly));
	inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	Com_Memset(&viewportState, 0, sizeof(viewportState));
	viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount  = 1;

	Com_Memset(&rasterizer, 0, sizeof(rasterizer));
	rasterizer.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode    = VK_CULL_MODE_NONE;
	rasterizer.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.lineWidth   = 1.0f;

	Com_Memset(&multisampling, 0, sizeof(multisampling));
	multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	Com_Memset(&colorBlendAttachment, 0, sizeof(colorBlendAttachment));
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	                                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	Com_Memset(&colorBlending, 0, sizeof(colorBlending));
	colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments    = &colorBlendAttachment;

	Com_Memset(&dynamicState, 0, sizeof(dynamicState));
	dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates    = dynamicStates;

	Com_Memset(&pipelineInfo, 0, sizeof(pipelineInfo));
	pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount          = 2;
	pipelineInfo.pStages             = stages;
	pipelineInfo.pVertexInputState   = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState      = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.layout              = vk.gammaPipelineLayout;
	pipelineInfo.renderPass          = vk.presentRenderPass;
	pipelineInfo.subpass             = 0;

	if (!VK_CheckResult(vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vk.gammaPipeline), "vkCreateGraphicsPipelines(gamma)"))
	{
		return qfalse;
	}

	Com_Memset(&dsAllocInfo, 0, sizeof(dsAllocInfo));
	dsAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	dsAllocInfo.descriptorPool     = vk.uiDescriptorPool;
	dsAllocInfo.descriptorSetCount = 1;
	dsAllocInfo.pSetLayouts        = &vk.uiDescriptorSetLayout;

	if (!VK_CheckResult(vkAllocateDescriptorSets(vk.device, &dsAllocInfo, &vk.sceneColorDescriptor), "vkAllocateDescriptorSets(scene color)"))
	{
		return qfalse;
	}

	Com_Memset(&imageDescInfo, 0, sizeof(imageDescInfo));
	imageDescInfo.sampler     = vk.uiSamplerClamp;
	imageDescInfo.imageView   = vk.sceneColorView;
	imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	Com_Memset(&writeSet, 0, sizeof(writeSet));
	writeSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.dstSet          = vk.sceneColorDescriptor;
	writeSet.dstBinding      = 0;
	writeSet.descriptorCount = 1;
	writeSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeSet.pImageInfo      = &imageDescInfo;

	vkUpdateDescriptorSets(vk.device, 1, &writeSet, 0, NULL);

	return qtrue;
}

static qboolean VK_CreateCommandObjects(void)
{
	VkCommandPoolCreateInfo     poolInfo;
	VkCommandBufferAllocateInfo allocInfo;
	VkSemaphoreCreateInfo       semaphoreInfo;
	VkFenceCreateInfo           fenceInfo;
	int                         i;

	Com_Memset(&poolInfo, 0, sizeof(poolInfo));
	poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = vk.graphicsQueueFamily;

	if (!VK_CheckResult(vkCreateCommandPool(vk.device, &poolInfo, NULL, &vk.commandPool), "vkCreateCommandPool"))
	{
		return qfalse;
	}

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool        = vk.commandPool;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = VK_FRAMES_IN_FLIGHT;

	if (!VK_CheckResult(vkAllocateCommandBuffers(vk.device, &allocInfo, vk.commandBuffers), "vkAllocateCommandBuffers"))
	{
		return qfalse;
	}
	vk.commandBuffer = vk.commandBuffers[0];
	vk.frameIndex    = VK_FRAMES_IN_FLIGHT - 1; // VK_BeginFrame() advances to slot 0 first

	Com_Memset(&semaphoreInfo, 0, sizeof(semaphoreInfo));
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	Com_Memset(&fenceInfo, 0, sizeof(fenceInfo));
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (i = 0; i < VK_FRAMES_IN_FLIGHT; i++)
	{
		if (!VK_CheckResult(vkCreateSemaphore(vk.device, &semaphoreInfo, NULL, &vk.imageAvailableSemaphores[i]), "vkCreateSemaphore(imageAvailable)") ||
		    !VK_CheckResult(vkCreateFence(vk.device, &fenceInfo, NULL, &vk.inFlightFences[i]), "vkCreateFence"))
		{
			return qfalse;
		}
	}
	for (i = 0; i < VK_MAX_SWAPCHAIN_IMAGES; i++)
	{
		if (!VK_CheckResult(vkCreateSemaphore(vk.device, &semaphoreInfo, NULL, &vk.renderFinishedSemaphores[i]), "vkCreateSemaphore(renderFinished)"))
		{
			return qfalse;
		}
	}
	vk.imageAvailableSemaphore = vk.imageAvailableSemaphores[0];
	vk.renderFinishedSemaphore = vk.renderFinishedSemaphores[0];
	vk.inFlightFence           = vk.inFlightFences[0];

	return qtrue;
}

/**
 * @brief Finds a device memory type index matching both the given
 * memory-type bitmask (from a VkMemoryRequirements) and the requested
 * property flags (host-visible, device-local, ...).
 */
static uint32_t VK_FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	uint32_t                         i;

	vkGetPhysicalDeviceMemoryProperties(vk.physicalDevice, &memProperties);

	for (i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	Ren_Print("Vulkan: failed to find a suitable memory type\n");
	return 0;
}

/**
 * @brief Allocates a VkBuffer + backing VkDeviceMemory pair with the given
 * usage/property flags.
 */
static qboolean VK_CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *memory)
{
	VkBufferCreateInfo   bufferInfo;
	VkMemoryRequirements memRequirements;
	VkMemoryAllocateInfo allocInfo;

	Com_Memset(&bufferInfo, 0, sizeof(bufferInfo));
	bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size        = size;
	bufferInfo.usage       = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (!VK_CheckResult(vkCreateBuffer(vk.device, &bufferInfo, NULL, buffer), "vkCreateBuffer"))
	{
		return qfalse;
	}

	vkGetBufferMemoryRequirements(vk.device, *buffer, &memRequirements);

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = VK_FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (!VK_CheckResult(vkAllocateMemory(vk.device, &allocInfo, NULL, memory), "vkAllocateMemory"))
	{
		vkDestroyBuffer(vk.device, *buffer, NULL);
		*buffer = VK_NULL_HANDLE;
		return qfalse;
	}

	vkBindBufferMemory(vk.device, *buffer, *memory, 0);

	return qtrue;
}

/**
 * @brief Begins a single-use primary command buffer for one-shot transfer
 * operations (texture upload layout transitions/copies). The caller must
 * pair this with VK_EndOneShotCommands().
 */
static VkCommandBuffer VK_BeginOneShotCommands(void)
{
	VkCommandBufferAllocateInfo allocInfo;
	VkCommandBufferBeginInfo    beginInfo;
	VkCommandBuffer             commandBuffer;

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool        = vk.commandPool;
	allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(vk.device, &allocInfo, &commandBuffer);

	Com_Memset(&beginInfo, 0, sizeof(beginInfo));
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

/**
 * @brief Ends and submits a one-shot command buffer created with
 * VK_BeginOneShotCommands(), waiting (vkQueueWaitIdle) for it to complete
 * before freeing it. Synchronous and only meant for load-time texture
 * uploads, not per-frame work.
 */
static void VK_EndOneShotCommands(VkCommandBuffer commandBuffer)
{
	VkSubmitInfo submitInfo;

	vkEndCommandBuffer(commandBuffer);

	Com_Memset(&submitInfo, 0, sizeof(submitInfo));
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &commandBuffer;

	vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vk.graphicsQueue);

	vkFreeCommandBuffers(vk.device, vk.commandPool, 1, &commandBuffer);
}

/**
 * @brief Creates one sampler for mipmapped images from the
 * current GL-style texture mode (renderer1's GL_TextureMode() /
 * glTexParameter MIN/MAG filter + GL_TEXTURE_MAX_ANISOTROPY_EXT).
 * Non-mipmap min filters (GL_NEAREST/GL_LINEAR) clamp to level 0 via
 * maxLod = 0.25, the spec-recommended GL emulation.
 */
static VkSampler VK_CreateMipSampler(VkSamplerAddressMode addressMode)
{
	VkSamplerCreateInfo samplerInfo;
	VkSampler           sampler = VK_NULL_HANDLE;
	qboolean            useMips = qtrue;

	Com_Memset(&samplerInfo, 0, sizeof(samplerInfo));
	samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter    = (vk.texFilterMag == GL_NEAREST) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
	samplerInfo.addressModeU = addressMode;
	samplerInfo.addressModeV = addressMode;
	samplerInfo.addressModeW = addressMode;
	samplerInfo.borderColor  = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	switch (vk.texFilterMin)
	{
	case GL_NEAREST:
		samplerInfo.minFilter  = VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		useMips                = qfalse;
		break;
	case GL_LINEAR:
		samplerInfo.minFilter  = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		useMips                = qfalse;
		break;
	case GL_NEAREST_MIPMAP_NEAREST:
		samplerInfo.minFilter  = VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		break;
	case GL_NEAREST_MIPMAP_LINEAR:
		samplerInfo.minFilter  = VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		break;
	case GL_LINEAR_MIPMAP_LINEAR:
		samplerInfo.minFilter  = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		break;
	case GL_LINEAR_MIPMAP_NEAREST:
	default:
		samplerInfo.minFilter  = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		break;
	}

	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = useMips ? VK_LOD_CLAMP_NONE : 0.25f;

	if (useMips && vk.maxSamplerAnisotropy > 1.0f && r_extTextureFilterAnisotropic->value > 1.0f)
	{
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy    = Com_Clamp(1.0f, vk.maxSamplerAnisotropy, r_extTextureFilterAnisotropic->value);
	}

	if (!VK_CheckResult(vkCreateSampler(vk.device, &samplerInfo, NULL, &sampler), "vkCreateSampler(mip)"))
	{
		return VK_NULL_HANDLE;
	}

	return sampler;
}

static qboolean VK_CreateMipSamplers(void)
{
	vk.mipSamplerRepeat = VK_CreateMipSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT);
	vk.mipSamplerClamp  = VK_CreateMipSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	return (vk.mipSamplerRepeat != VK_NULL_HANDLE && vk.mipSamplerClamp != VK_NULL_HANDLE) ? qtrue : qfalse;
}

static VkSampler VK_SamplerForImage(qboolean mipmap, int wrapClampMode)
{
	if (mipmap)
	{
		return (wrapClampMode == GL_REPEAT) ? vk.mipSamplerRepeat : vk.mipSamplerClamp;
	}

	return (wrapClampMode == GL_REPEAT) ? vk.uiSamplerRepeat : vk.uiSamplerClamp;
}

/**
 * @brief Creates the two samplers used by non-mipmapped images (one per
 * wrap mode), always LINEAR filtered and clamped to level 0.
 */
static qboolean VK_CreateUISamplers(void)
{
	VkSamplerCreateInfo samplerInfo;

	Com_Memset(&samplerInfo, 0, sizeof(samplerInfo));
	samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter    = VK_FILTER_LINEAR;
	samplerInfo.minFilter    = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.maxLod       = 0.25f;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.borderColor  = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	if (!VK_CheckResult(vkCreateSampler(vk.device, &samplerInfo, NULL, &vk.uiSamplerRepeat), "vkCreateSampler(repeat)"))
	{
		return qfalse;
	}

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	if (!VK_CheckResult(vkCreateSampler(vk.device, &samplerInfo, NULL, &vk.uiSamplerClamp), "vkCreateSampler(clamp)"))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Creates the descriptor pool that every 2D image's descriptor set
 * (one combined-image-sampler each) is allocated from.
 */
static qboolean VK_CreateUIDescriptorPool(void)
{
	VkDescriptorPoolSize       poolSize;
	VkDescriptorPoolCreateInfo poolInfo;

	Com_Memset(&poolSize, 0, sizeof(poolSize));
	poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = VK_UI_MAX_DESCRIPTOR_SETS;

	Com_Memset(&poolInfo, 0, sizeof(poolInfo));
	poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes    = &poolSize;
	poolInfo.maxSets       = VK_UI_MAX_DESCRIPTOR_SETS;

	return VK_CheckResult(vkCreateDescriptorPool(vk.device, &poolInfo, NULL, &vk.uiDescriptorPool), "vkCreateDescriptorPool");
}

/**
 * @brief Creates a VkShaderModule from an embedded SPIR-V byte array.
 */
static VkShaderModule VK_CreateShaderModule(const unsigned char *code, unsigned int size)
{
	VkShaderModuleCreateInfo createInfo;
	VkShaderModule           module = VK_NULL_HANDLE;

	Com_Memset(&createInfo, 0, sizeof(createInfo));
	createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = size;
	createInfo.pCode    = (const uint32_t *)code;

	VK_CheckResult(vkCreateShaderModule(vk.device, &createInfo, NULL, &module), "vkCreateShaderModule");

	return module;
}

/**
 * @brief Maps the engine's GLS_SRCBLEND_* / GLS_DSTBLEND_* state bits to the
 * matching VkBlendFactor, mirroring GL_State()'s switch in tr_backend.c.
 */
static VkBlendFactor VK_SrcBlendFactor(unsigned long stateBits)
{
	switch (stateBits & GLS_SRCBLEND_BITS)
	{
	case GLS_SRCBLEND_ZERO: return VK_BLEND_FACTOR_ZERO;
	case GLS_SRCBLEND_ONE: return VK_BLEND_FACTOR_ONE;
	case GLS_SRCBLEND_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
	case GLS_SRCBLEND_ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case GLS_SRCBLEND_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
	case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case GLS_SRCBLEND_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
	case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case GLS_SRCBLEND_ALPHA_SATURATE: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
	default: return VK_BLEND_FACTOR_ONE;
	}
}

static VkBlendFactor VK_DstBlendFactor(unsigned long stateBits)
{
	switch (stateBits & GLS_DSTBLEND_BITS)
	{
	case GLS_DSTBLEND_ZERO: return VK_BLEND_FACTOR_ZERO;
	case GLS_DSTBLEND_ONE: return VK_BLEND_FACTOR_ONE;
	case GLS_DSTBLEND_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
	case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case GLS_DSTBLEND_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
	case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case GLS_DSTBLEND_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
	case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	default: return VK_BLEND_FACTOR_ZERO;
	}
}

/**
 * @brief Creates the descriptor set layout (single combined-image-sampler),
 * push-constant (scale/offset + alphaTest) pipeline layout, and compiles
 * the shader modules shared by every lazily-created UI pipeline variant
 * (see VK_GetUIPipeline()). Variants honor each
 * shader stage's real stateBits (alpha test, depth test/func/write)
 * instead of always forcing RB_SetGL2D's fixed blend-only state - this is
 * required for tricks like the automap's circular-mask (alphafunc GE128 +
 * depthFunc equal) in cg_commandmap.c to render correctly.
 */
/**
 * @brief Bindless textures (vk.bindless): the set layout (one partially
 * bound, update-after-bind array of VK_BINDLESS_MAX_IMAGES combined image
 * samplers), its pool and single set, the 2D pipeline layout using it, the
 * bindless UI shaders and the free slot list. Images are written into it by
 * VK_CreateImageTexture (VK_BindlessWrite).
 */
static qboolean VK_CreateBindlessResources(const VkPushConstantRange *pushConstantRange)
{
	VkDescriptorSetLayoutBinding                binding;
	VkDescriptorBindingFlags                    bindingFlags;
	VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo;
	VkDescriptorSetLayoutCreateInfo             layoutInfo;
	VkDescriptorPoolSize                        poolSize;
	VkDescriptorPoolCreateInfo                  poolInfo;
	VkDescriptorSetAllocateInfo                 allocInfo;
	VkPipelineLayoutCreateInfo                  pipelineLayoutInfo;
	int                                         i;

	Com_Memset(&binding, 0, sizeof(binding));
	binding.binding         = 0;
	binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount = VK_BINDLESS_MAX_IMAGES;
	binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

	// slots are written while frames using other slots are in flight
	bindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
	               VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;

	Com_Memset(&flagsInfo, 0, sizeof(flagsInfo));
	flagsInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	flagsInfo.bindingCount  = 1;
	flagsInfo.pBindingFlags = &bindingFlags;

	Com_Memset(&layoutInfo, 0, sizeof(layoutInfo));
	layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext        = &flagsInfo;
	layoutInfo.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings    = &binding;

	if (!VK_CheckResult(vkCreateDescriptorSetLayout(vk.device, &layoutInfo, NULL, &vk.bindlessSetLayout), "vkCreateDescriptorSetLayout(bindless)"))
	{
		return qfalse;
	}

	Com_Memset(&poolSize, 0, sizeof(poolSize));
	poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = VK_BINDLESS_MAX_IMAGES;

	Com_Memset(&poolInfo, 0, sizeof(poolInfo));
	poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes    = &poolSize;
	poolInfo.maxSets       = 1;

	if (!VK_CheckResult(vkCreateDescriptorPool(vk.device, &poolInfo, NULL, &vk.bindlessPool), "vkCreateDescriptorPool(bindless)"))
	{
		return qfalse;
	}

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool     = vk.bindlessPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts        = &vk.bindlessSetLayout;

	if (!VK_CheckResult(vkAllocateDescriptorSets(vk.device, &allocInfo, &vk.bindlessSet), "vkAllocateDescriptorSets(bindless)"))
	{
		return qfalse;
	}

	Com_Memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
	pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount         = 1;
	pipelineLayoutInfo.pSetLayouts            = &vk.bindlessSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges    = pushConstantRange;

	if (!VK_CheckResult(vkCreatePipelineLayout(vk.device, &pipelineLayoutInfo, NULL, &vk.uiBindlessPipelineLayout), "vkCreatePipelineLayout(bindless)"))
	{
		return qfalse;
	}

	vk.uiBindlessVertModule = VK_CreateShaderModule(ui_bindless_vert_spv, ui_bindless_vert_spv_SIZE);
	vk.uiBindlessFragModule = VK_CreateShaderModule(ui_bindless_frag_spv, ui_bindless_frag_spv_SIZE);
	if (vk.uiBindlessVertModule == VK_NULL_HANDLE || vk.uiBindlessFragModule == VK_NULL_HANDLE)
	{
		return qfalse;
	}

	// slot 0 is handed out first
	vk.numBindlessFree = 0;
	for (i = VK_BINDLESS_MAX_IMAGES - 1; i >= 0; i--)
	{
		vk.bindlessFree[vk.numBindlessFree++] = i;
	}

	return qtrue;
}

/**
 * @brief Writes an image's view and sampler into its bindless slot.
 */
static void VK_BindlessWrite(int slot, VkImageView view, VkSampler sampler)
{
	VkDescriptorImageInfo imageInfo;
	VkWriteDescriptorSet  write;

	Com_Memset(&imageInfo, 0, sizeof(imageInfo));
	imageInfo.sampler     = sampler;
	imageInfo.imageView   = view;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	Com_Memset(&write, 0, sizeof(write));
	write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet          = vk.bindlessSet;
	write.dstBinding      = 0;
	write.dstArrayElement = (uint32_t)slot;
	write.descriptorCount = 1;
	write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.pImageInfo      = &imageInfo;

	vkUpdateDescriptorSets(vk.device, 1, &write, 0, NULL);
}

static qboolean VK_CreateUIPipeline(void)
{
	VkDescriptorSetLayoutBinding    binding;
	VkDescriptorSetLayoutCreateInfo layoutInfo;
	VkPushConstantRange             pushConstantRange;
	VkPipelineLayoutCreateInfo      pipelineLayoutInfo;

	Com_Memset(&binding, 0, sizeof(binding));
	binding.binding         = 0;
	binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount = 1;
	binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

	Com_Memset(&layoutInfo, 0, sizeof(layoutInfo));
	layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings    = &binding;

	if (!VK_CheckResult(vkCreateDescriptorSetLayout(vk.device, &layoutInfo, NULL, &vk.uiDescriptorSetLayout), "vkCreateDescriptorSetLayout"))
	{
		return qfalse;
	}

	Com_Memset(&pushConstantRange, 0, sizeof(pushConstantRange));
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset     = 0;
	pushConstantRange.size       = sizeof(float) * 8; // vec2 scale + vec2 offset + vec4 alphaTest

	Com_Memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
	pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount         = 1;
	pipelineLayoutInfo.pSetLayouts            = &vk.uiDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

	if (!VK_CheckResult(vkCreatePipelineLayout(vk.device, &pipelineLayoutInfo, NULL, &vk.uiPipelineLayout), "vkCreatePipelineLayout"))
	{
		return qfalse;
	}

	vk.uiVertModule = VK_CreateShaderModule(ui_vert_spv, ui_vert_spv_SIZE);
	vk.uiFragModule = VK_CreateShaderModule(ui_frag_spv, ui_frag_spv_SIZE);

	if (vk.uiVertModule == VK_NULL_HANDLE || vk.uiFragModule == VK_NULL_HANDLE)
	{
		return qfalse;
	}

	if (vk.bindless && !VK_CreateBindlessResources(&pushConstantRange))
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Creates one 2D/UI pipeline variant for the given stateBits,
 * honoring depth test/write/func and blend state exactly like the
 * scene pipelines (see VK_CreateScenePipelineVariant()).
 */
static VkPipeline VK_CreateUIPipelineVariant(unsigned long stateBits)
{
	VkPipelineShaderStageCreateInfo        stages[2];
	VkVertexInputBindingDescription        vertexBinding;
	VkVertexInputAttributeDescription      vertexAttributes[4];
	VkPipelineVertexInputStateCreateInfo   vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkPipelineViewportStateCreateInfo      viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo   multisampling;
	VkPipelineDepthStencilStateCreateInfo  depthStencil;
	VkPipelineColorBlendAttachmentState    colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo    colorBlending;
	VkDynamicState                         dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo       dynamicState;
	VkGraphicsPipelineCreateInfo           pipelineInfo;
	VkPipeline                             pipeline = VK_NULL_HANDLE;

	Com_Memset(stages, 0, sizeof(stages));
	stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vk.bindless ? vk.uiBindlessVertModule : vk.uiVertModule;
	stages[0].pName  = "main";
	stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = vk.bindless ? vk.uiBindlessFragModule : vk.uiFragModule;
	stages[1].pName  = "main";

	Com_Memset(&vertexBinding, 0, sizeof(vertexBinding));
	vertexBinding.binding   = 0;
	vertexBinding.stride    = sizeof(vkUIVertex_t);
	vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	Com_Memset(vertexAttributes, 0, sizeof(vertexAttributes));
	vertexAttributes[0].location = 0;
	vertexAttributes[0].binding  = 0;
	vertexAttributes[0].format   = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributes[0].offset   = offsetof(vkUIVertex_t, pos);
	vertexAttributes[1].location = 1;
	vertexAttributes[1].binding  = 0;
	vertexAttributes[1].format   = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributes[1].offset   = offsetof(vkUIVertex_t, uv);
	vertexAttributes[2].location = 2;
	vertexAttributes[2].binding  = 0;
	vertexAttributes[2].format   = VK_FORMAT_R8G8B8A8_UNORM;
	vertexAttributes[2].offset   = offsetof(vkUIVertex_t, color);
	vertexAttributes[3].location = 3;
	vertexAttributes[3].binding  = 0;
	vertexAttributes[3].format   = VK_FORMAT_R32_UINT;
	vertexAttributes[3].offset   = offsetof(vkUIVertex_t, texture);

	Com_Memset(&vertexInputInfo, 0, sizeof(vertexInputInfo));
	vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount   = 1;
	vertexInputInfo.pVertexBindingDescriptions      = &vertexBinding;
	vertexInputInfo.vertexAttributeDescriptionCount = vk.bindless ? 4 : 3;
	vertexInputInfo.pVertexAttributeDescriptions    = vertexAttributes;

	Com_Memset(&inputAssembly, 0, sizeof(inputAssembly));
	inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	Com_Memset(&viewportState, 0, sizeof(viewportState));
	viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount  = 1;

	Com_Memset(&rasterizer, 0, sizeof(rasterizer));
	rasterizer.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode    = VK_CULL_MODE_NONE; // RB_SetGL2D: glDisable(GL_CULL_FACE)
	rasterizer.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.lineWidth   = 1.0f;

	Com_Memset(&multisampling, 0, sizeof(multisampling));
	multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = vk.sampleCount; // MSAA

	Com_Memset(&depthStencil, 0, sizeof(depthStencil));
	depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable  = (stateBits & GLS_DEPTHTEST_DISABLE) ? VK_FALSE : VK_TRUE;
	depthStencil.depthWriteEnable = (stateBits & GLS_DEPTHMASK_TRUE) ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp   = (stateBits & GLS_DEPTHFUNC_EQUAL) ? VK_COMPARE_OP_EQUAL : VK_COMPARE_OP_LESS_OR_EQUAL;

	Com_Memset(&colorBlendAttachment, 0, sizeof(colorBlendAttachment));
	colorBlendAttachment.blendEnable         = (stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) ? VK_TRUE : VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_SrcBlendFactor(stateBits);
	colorBlendAttachment.dstColorBlendFactor = VK_DstBlendFactor(stateBits);
	colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_SrcBlendFactor(stateBits);
	colorBlendAttachment.dstAlphaBlendFactor = VK_DstBlendFactor(stateBits);
	colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	Com_Memset(&colorBlending, 0, sizeof(colorBlending));
	colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments    = &colorBlendAttachment;

	Com_Memset(&dynamicState, 0, sizeof(dynamicState));
	dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates    = dynamicStates;

	Com_Memset(&pipelineInfo, 0, sizeof(pipelineInfo));
	pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount          = 2;
	pipelineInfo.pStages             = stages;
	pipelineInfo.pVertexInputState   = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState      = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pDepthStencilState  = &depthStencil;
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.layout              = vk.bindless ? vk.uiBindlessPipelineLayout : vk.uiPipelineLayout;
	pipelineInfo.renderPass          = vk.renderPass;
	pipelineInfo.subpass             = 0;

	if (!VK_CheckResult(vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline), "vkCreateGraphicsPipelines(ui)"))
	{
		return VK_NULL_HANDLE;
	}

	return pipeline;
}

/**
 * @brief Returns the cached 2D/UI pipeline for this stateBits value,
 * creating and caching it on first use. Returns VK_NULL_HANDLE if the
 * cache is full or creation failed (caller should skip the draw).
 */
static VkPipeline VK_GetUIPipeline(unsigned long stateBits)
{
	int        i;
	VkPipeline pipeline;

	for (i = 0; i < vk.numUIPipelines; i++)
	{
		if (vk.uiPipelines[i].stateBits == stateBits)
		{
			return vk.uiPipelines[i].pipeline;
		}
	}

	if (vk.numUIPipelines >= VK_MAX_SCENE_PIPELINES)
	{
		Ren_Print("Vulkan: UI pipeline cache full, dropping draw\n");
		return VK_NULL_HANDLE;
	}

	pipeline = VK_CreateUIPipelineVariant(stateBits);
	if (pipeline == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE;
	}

	vk.uiPipelines[vk.numUIPipelines].stateBits = stateBits;
	vk.uiPipelines[vk.numUIPipelines].pipeline  = pipeline;
	vk.numUIPipelines++;

	return pipeline;
}

/**
 * @brief Creates the persistently-mapped vertex/index arena buffers used
 * by VK_Draw2D() for every 2D draw this frame.
 */
static qboolean VK_CreateUIArenaBuffers(void)
{
	// one per-frame region per frame slot; VK_BeginFrame() narrows
	// offset/size to the current slot's region
	vk.vertexArenaSize = (VkDeviceSize)VK_UI_VERTEX_ARENA_SIZE * VK_FRAMES_IN_FLIGHT;
	vk.indexArenaSize  = (VkDeviceSize)VK_UI_INDEX_ARENA_SIZE * VK_FRAMES_IN_FLIGHT;

	if (!VK_CreateBuffer(vk.vertexArenaSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &vk.vertexArenaBuffer, &vk.vertexArenaMemory))
	{
		return qfalse;
	}
	if (!VK_CreateBuffer(vk.indexArenaSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &vk.indexArenaBuffer, &vk.indexArenaMemory))
	{
		return qfalse;
	}

	vkMapMemory(vk.device, vk.vertexArenaMemory, 0, vk.vertexArenaSize, 0, &vk.vertexArenaMapped);
	vkMapMemory(vk.device, vk.indexArenaMemory, 0, vk.indexArenaSize, 0, &vk.indexArenaMapped);

	return qtrue;
}

/**
 * @brief Persistently-mapped vertex/index arena buffers for
 * 3D scene draws, analogous to the UI arena buffers but with their own
 * (larger) backing storage and vertex formats.
 */
/**
 * @brief GPU skinning resources: the skin vertex buffer (VK_WriteSkinVertices),
 * the per-frame bone arena (VK_AllocSkinBones) and the descriptor set that
 * reads it as a dynamic uniform buffer of VK_SKIN_MAX_BONES bones. Needs
 * vk.boneSetLayout (VK_CreateScenePipelineLayouts).
 */
static qboolean VK_CreateSkinResources(void)
{
	VkPhysicalDeviceProperties  props;
	VkDescriptorPoolSize        poolSize;
	VkDescriptorPoolCreateInfo  poolInfo;
	VkDescriptorSetAllocateInfo allocInfo;
	VkDescriptorBufferInfo      bufferInfo;
	VkWriteDescriptorSet        write;
	VkDeviceSize                range = MAX((VkDeviceSize)VK_SKIN_MAX_BONES * VK_SKIN_BONE_BYTES, VK_UNIFORM_RANGE);

	vkGetPhysicalDeviceProperties(vk.physicalDevice, &props);
	vk.uboAlignment = MAX(props.limits.minUniformBufferOffsetAlignment, 16);

	vk.skinMaxVertexes = VK_SKIN_BUFFER_SIZE / sizeof(vkSkinVertex_t);
	if (!VK_CreateBuffer((VkDeviceSize)vk.skinMaxVertexes * sizeof(vkSkinVertex_t), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &vk.skinBuffer, &vk.skinMemory) ||
	    !VK_CheckResult(vkMapMemory(vk.device, vk.skinMemory, 0, VK_WHOLE_SIZE, 0, &vk.skinMapped), "vkMapMemory(skin)"))
	{
		return qfalse;
	}
	vk.skinNumVertexes = 0;

	if (!VK_CreateBuffer(VK_MESH_BUFFER_SIZE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &vk.meshBuffer, &vk.meshMemory) ||
	    !VK_CheckResult(vkMapMemory(vk.device, vk.meshMemory, 0, VK_WHOLE_SIZE, 0, &vk.meshMapped), "vkMapMemory(mesh)"))
	{
		return qfalse;
	}
	vk.meshBytes       = 0;
	vk.meshTablesFrame = -1;

	// a full bone range must fit after the last frame slot's region
	if (!VK_CreateBuffer((VkDeviceSize)VK_SKIN_BONE_ARENA_SIZE * VK_FRAMES_IN_FLIGHT + range,
	                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &vk.boneArenaBuffer, &vk.boneArenaMemory) ||
	    !VK_CheckResult(vkMapMemory(vk.device, vk.boneArenaMemory, 0, VK_WHOLE_SIZE, 0, &vk.boneArenaMapped), "vkMapMemory(bones)"))
	{
		return qfalse;
	}

	Com_Memset(&poolSize, 0, sizeof(poolSize));
	poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	poolSize.descriptorCount = 1;

	Com_Memset(&poolInfo, 0, sizeof(poolInfo));
	poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes    = &poolSize;
	poolInfo.maxSets       = 1;

	if (!VK_CheckResult(vkCreateDescriptorPool(vk.device, &poolInfo, NULL, &vk.bonePool), "vkCreateDescriptorPool(bones)"))
	{
		return qfalse;
	}

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool     = vk.bonePool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts        = &vk.boneSetLayout;

	if (!VK_CheckResult(vkAllocateDescriptorSets(vk.device, &allocInfo, &vk.boneSet), "vkAllocateDescriptorSets(bones)"))
	{
		return qfalse;
	}

	Com_Memset(&bufferInfo, 0, sizeof(bufferInfo));
	bufferInfo.buffer = vk.boneArenaBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range  = range;

	Com_Memset(&write, 0, sizeof(write));
	write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet          = vk.boneSet;
	write.dstBinding      = 0;
	write.descriptorCount = 1;
	write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	write.pBufferInfo     = &bufferInfo;
	vkUpdateDescriptorSets(vk.device, 1, &write, 0, NULL);

	return qtrue;
}

/**
 * @brief Forgets all skin vertices (R_ModelInit: the models are loaded
 * again). Waits for the GPU, which may still draw the old ones.
 */
void VK_ResetSkinBuffer(void)
{
	if (!vk.initialized || (!vk.skinNumVertexes && !vk.meshBytes))
	{
		return;
	}

	vkDeviceWaitIdle(vk.device);
	vk.skinNumVertexes = 0;
	vk.meshBytes       = 0;
}

/**
 * @brief Appends bytes of MD3 data (frames, texture coords or triangles of
 * one surface, R_VkMeshRegister) to the mesh buffer, 16-byte aligned.
 * @return its byte offset, or -1 if it doesn't fit (the surface is then
 * interpolated on the CPU)
 */
int VK_WriteMeshData(const void *data, int bytes)
{
	int offset;

	if (!vk.initialized || !vk.meshMapped || bytes <= 0)
	{
		return -1;
	}

	offset = (vk.meshBytes + 15) & ~15;
	if (offset + bytes > VK_MESH_BUFFER_SIZE)
	{
		Ren_Developer("Vulkan: mesh buffer full, MD3 surface interpolated on the CPU\n");
		return -1;
	}

	Com_Memcpy((byte *)vk.meshMapped + offset, data, (size_t)bytes);
	vk.meshBytes = offset + bytes;

	return offset;
}

/**
 * @brief Appends count skin vertices (one MDM surface, R_LoadMDMSkinVertices).
 * @return the first vertex's index, or -1 if they don't fit (the surface is
 * then skinned on the CPU)
 */
int VK_WriteSkinVertices(const vkSkinVertex_t *verts, int count)
{
	int first;

	if (!vk.initialized || !vk.skinMapped || count <= 0)
	{
		return -1;
	}

	if (vk.skinNumVertexes + count > vk.skinMaxVertexes)
	{
		Ren_Developer("Vulkan: skin buffer full, MDM surface skinned on the CPU\n");
		return -1;
	}

	first = vk.skinNumVertexes;
	Com_Memcpy((vkSkinVertex_t *)vk.skinMapped + first, verts, sizeof(*verts) * count);
	vk.skinNumVertexes += count;

	return first;
}

/**
 * @brief Reserves bytes of this frame's uniform arena (the bone arena: GPU
 * skinning bones, folded dlights), read through vk.boneSet at a dynamic
 * offset. Blocks must fit VK_SKIN_MAX_BONES bones (the descriptor's range).
 * @param[out] offset dynamic offset of the block (bytes)
 * @return write pointer, or NULL if the arena is exhausted this frame
 */
static float *VK_AllocUniforms(VkDeviceSize bytes, unsigned int *offset)
{
	VkDeviceSize start = ((vk.boneArenaOffset + vk.uboAlignment - 1) / vk.uboAlignment) * vk.uboAlignment;

	if (!vk.initialized || start + bytes > vk.boneArenaSize)
	{
		static int lastWarnFrame = -1;

		if (vk.initialized && backEnd.refdef.time != lastWarnFrame)
		{
			Ren_Print("Vulkan: VK_AllocUniforms - uniform arena exhausted this frame, dropping draws\n");
			lastWarnFrame = backEnd.refdef.time;
		}
		return NULL;
	}

	*offset            = (unsigned int)start;
	vk.boneArenaOffset = start + bytes;

	return (float *)((byte *)vk.boneArenaMapped + start);
}

/**
 * @brief Reserves numBones bones (12 floats each: 3 rows of matrix +
 * translation) in this frame's bone arena for one GPU-skinned batch.
 * @param[out] offset dynamic offset of the block (bytes)
 * @return write pointer, or NULL if the arena is exhausted this frame
 */
float *VK_AllocSkinBones(int numBones, unsigned int *offset)
{
	if (numBones > VK_SKIN_MAX_BONES)
	{
		return NULL;
	}

	return VK_AllocUniforms((VkDeviceSize)numBones * VK_SKIN_BONE_BYTES, offset);
}

/**
 * @brief Reserves one GPU-skinned/animated batch's tcGen environment
 * parameters (RB_VkSetupEnvParams: 3 vec4s) in this frame's uniform arena.
 * @param[out] offset dynamic offset of the block (bytes)
 * @return write pointer, or NULL if the arena is exhausted this frame
 */
float *VK_AllocEnvParams(unsigned int *offset)
{
	return VK_AllocUniforms(12 * sizeof(float), offset);
}

static qboolean VK_CreateSceneArenaBuffers(void)
{
	vk.sceneVertexArenaSize = (VkDeviceSize)VK_SCENE_VERTEX_ARENA_SIZE * VK_FRAMES_IN_FLIGHT;
	vk.sceneIndexArenaSize  = (VkDeviceSize)VK_SCENE_INDEX_ARENA_SIZE * VK_FRAMES_IN_FLIGHT;

	if (!VK_CreateBuffer(vk.sceneVertexArenaSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &vk.sceneVertexArenaBuffer, &vk.sceneVertexArenaMemory))
	{
		return qfalse;
	}
	if (!VK_CreateBuffer(vk.sceneIndexArenaSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &vk.sceneIndexArenaBuffer, &vk.sceneIndexArenaMemory))
	{
		return qfalse;
	}

	vkMapMemory(vk.device, vk.sceneVertexArenaMemory, 0, vk.sceneVertexArenaSize, 0, &vk.sceneVertexArenaMapped);
	vkMapMemory(vk.device, vk.sceneIndexArenaMemory, 0, vk.sceneIndexArenaSize, 0, &vk.sceneIndexArenaMapped);

	return VK_CreateSkinResources();
}

/**
 * @brief Destroys the static world vertex buffer (waits for the GPU, only
 * called at map load and shutdown).
 */
static void VK_DestroyStaticWorldBuffer(void)
{
	if (vk.staticWorldBuffer == VK_NULL_HANDLE && vk.staticWorldMemory == VK_NULL_HANDLE)
	{
		return;
	}

	vkDeviceWaitIdle(vk.device);

	if (vk.staticWorldMapped)
	{
		vkUnmapMemory(vk.device, vk.staticWorldMemory);
	}
	if (vk.staticWorldBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(vk.device, vk.staticWorldBuffer, NULL);
	}
	if (vk.staticWorldMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(vk.device, vk.staticWorldMemory, NULL);
	}
	vk.staticWorldBuffer      = VK_NULL_HANDLE;
	vk.staticWorldMemory      = VK_NULL_HANDLE;
	vk.staticWorldMapped      = NULL;
	vk.staticWorldNumVertexes = 0;
}

/**
 * @brief Replaces the static world vertex buffer with one for numVertexes
 * vertices, filled with VK_WriteStaticWorldVertices(). Host-visible and
 * persistently mapped like the arenas (unified memory on Apple GPUs).
 */
qboolean VK_CreateStaticWorldBuffer(int numVertexes)
{
	if (!vk.initialized)
	{
		return qfalse;
	}

	VK_DestroyStaticWorldBuffer();

	if (numVertexes <= 0)
	{
		return qfalse;
	}

	if (!VK_CreateBuffer((VkDeviceSize)numVertexes * sizeof(vk3DVertexLM_t), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &vk.staticWorldBuffer, &vk.staticWorldMemory) ||
	    !VK_CheckResult(vkMapMemory(vk.device, vk.staticWorldMemory, 0, VK_WHOLE_SIZE, 0, &vk.staticWorldMapped), "vkMapMemory(static world)"))
	{
		VK_DestroyStaticWorldBuffer();
		return qfalse;
	}

	vk.staticWorldNumVertexes = numVertexes;
	return qtrue;
}

/**
 * @brief Copies BSP vertices (xyz, texture and lightmap coords, color) into
 * the static world buffer starting at vertex `first`.
 */
void VK_WriteStaticWorldVertices(int first, const drawVert_t *verts, int count)
{
	vk3DVertexLM_t *dst;
	int            i;

	if (!vk.staticWorldMapped || first < 0 || first + count > vk.staticWorldNumVertexes)
	{
		return;
	}

	dst = (vk3DVertexLM_t *)vk.staticWorldMapped + first;
	for (i = 0; i < count; i++, dst++, verts++)
	{
		dst->pos[0] = verts->xyz[0];
		dst->pos[1] = verts->xyz[1];
		dst->pos[2] = verts->xyz[2];
		dst->uv0[0] = verts->st[0];
		dst->uv0[1] = verts->st[1];
		dst->uv1[0] = verts->lightmap[0];
		dst->uv1[1] = verts->lightmap[1];
		Com_Memcpy(dst->color, verts->color, 4);
	}
}

/**
 * @brief Copies a foliage surface's model (xyz, texture and lightmap coords)
 * into the static world buffer starting at vertex `first`. The color comes
 * from each instance (VK_VERTS_FOLIAGE).
 */
void VK_WriteStaticFoliageVertices(int first, const srfFoliage_t *srf)
{
	vk3DVertexLM_t *dst;
	int            i;

	if (!vk.staticWorldMapped || first < 0 || first + srf->numVerts > vk.staticWorldNumVertexes)
	{
		return;
	}

	dst = (vk3DVertexLM_t *)vk.staticWorldMapped + first;
	for (i = 0; i < srf->numVerts; i++, dst++)
	{
		dst->pos[0] = srf->xyz[i][0];
		dst->pos[1] = srf->xyz[i][1];
		dst->pos[2] = srf->xyz[i][2];
		dst->uv0[0] = srf->texCoords[i][0];
		dst->uv0[1] = srf->texCoords[i][1];
		dst->uv1[0] = srf->lmTexCoords[i][0];
		dst->uv1[1] = srf->lmTexCoords[i][1];
		Com_Memset(dst->color, 0xff, 4);
	}
}

/**
 * @brief GPU world culling: the compute pipeline, its descriptor set and the
 * per-frame command buffers the culling dispatches are recorded into (each
 * frame submits its cull command buffer ahead of the main one). Optional:
 * without it every view walks the world on the CPU.
 */
static void VK_CreateCullPipeline(void)
{
	VkDescriptorSetLayoutBinding    bindings[7];
	VkDescriptorSetLayoutCreateInfo setLayoutInfo;
	VkPushConstantRange             pushRange;
	VkPipelineLayoutCreateInfo      layoutInfo;
	VkDescriptorPoolSize            poolSize;
	VkDescriptorPoolCreateInfo      poolInfo;
	VkDescriptorSetAllocateInfo     allocInfo;
	VkCommandBufferAllocateInfo     cmdInfo;
	VkComputePipelineCreateInfo     pipelineInfo;
	VkShaderModule                  module;
	int                             i;

	// 0-4 world_cull.comp, 3/5/6 world_gather.comp
	Com_Memset(bindings, 0, sizeof(bindings));
	for (i = 0; i < 7; i++)
	{
		bindings[i].binding         = i;
		bindings[i].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[i].descriptorCount = 1;
		bindings[i].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
	}

	Com_Memset(&setLayoutInfo, 0, sizeof(setLayoutInfo));
	setLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setLayoutInfo.bindingCount = 7;
	setLayoutInfo.pBindings    = bindings;

	if (!VK_CheckResult(vkCreateDescriptorSetLayout(vk.device, &setLayoutInfo, NULL, &vk.cullSetLayout), "vkCreateDescriptorSetLayout(cull)"))
	{
		return;
	}

	Com_Memset(&pushRange, 0, sizeof(pushRange));
	pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushRange.size       = sizeof(vkCullPush_t);

	Com_Memset(&layoutInfo, 0, sizeof(layoutInfo));
	layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount         = 1;
	layoutInfo.pSetLayouts            = &vk.cullSetLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges    = &pushRange;

	if (!VK_CheckResult(vkCreatePipelineLayout(vk.device, &layoutInfo, NULL, &vk.cullPipelineLayout), "vkCreatePipelineLayout(cull)"))
	{
		return;
	}

	Com_Memset(&poolSize, 0, sizeof(poolSize));
	poolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSize.descriptorCount = 7;

	Com_Memset(&poolInfo, 0, sizeof(poolInfo));
	poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes    = &poolSize;
	poolInfo.maxSets       = 1;

	if (!VK_CheckResult(vkCreateDescriptorPool(vk.device, &poolInfo, NULL, &vk.cullPool), "vkCreateDescriptorPool(cull)"))
	{
		return;
	}

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool     = vk.cullPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts        = &vk.cullSetLayout;

	if (!VK_CheckResult(vkAllocateDescriptorSets(vk.device, &allocInfo, &vk.cullSet), "vkAllocateDescriptorSets(cull)"))
	{
		return;
	}

	Com_Memset(&cmdInfo, 0, sizeof(cmdInfo));
	cmdInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdInfo.commandPool        = vk.commandPool;
	cmdInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdInfo.commandBufferCount = VK_FRAMES_IN_FLIGHT;

	if (!VK_CheckResult(vkAllocateCommandBuffers(vk.device, &cmdInfo, vk.cullCommandBuffers), "vkAllocateCommandBuffers(cull)"))
	{
		return;
	}

	module = VK_CreateShaderModule(world_cull_comp_spv, world_cull_comp_spv_SIZE);
	if (module == VK_NULL_HANDLE)
	{
		return;
	}

	Com_Memset(&pipelineInfo, 0, sizeof(pipelineInfo));
	pipelineInfo.sType        = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipelineInfo.stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineInfo.stage.module = module;
	pipelineInfo.stage.pName  = "main";
	pipelineInfo.layout       = vk.cullPipelineLayout;

	VK_CheckResult(vkCreateComputePipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vk.cullPipeline), "vkCreateComputePipelines(cull)");
	vkDestroyShaderModule(vk.device, module, NULL);

	// merged world batches: gather pipeline (same layout), the per-draw
	// data set and the bindless static pipeline layout
	if (!vk.bindless || !vk.multiDrawIndirect)
	{
		return;
	}

	module = VK_CreateShaderModule(world_gather_comp_spv, world_gather_comp_spv_SIZE);
	if (module == VK_NULL_HANDLE)
	{
		return;
	}
	pipelineInfo.stage.module = module;
	VK_CheckResult(vkCreateComputePipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vk.gatherPipeline), "vkCreateComputePipelines(gather)");
	vkDestroyShaderModule(vk.device, module, NULL);

	{
		VkDescriptorSetLayout setLayouts[2];
		VkPushConstantRange   scenePush;

		Com_Memset(bindings, 0, sizeof(bindings));
		bindings[0].binding         = 0;
		bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
		setLayoutInfo.bindingCount  = 1;

		if (!VK_CheckResult(vkCreateDescriptorSetLayout(vk.device, &setLayoutInfo, NULL, &vk.worldDrawSetLayout), "vkCreateDescriptorSetLayout(world draws)"))
		{
			return;
		}

		poolSize.descriptorCount = 1;
		if (!VK_CheckResult(vkCreateDescriptorPool(vk.device, &poolInfo, NULL, &vk.worldDrawPool), "vkCreateDescriptorPool(world draws)"))
		{
			return;
		}
		allocInfo.descriptorPool = vk.worldDrawPool;
		allocInfo.pSetLayouts    = &vk.worldDrawSetLayout;
		if (!VK_CheckResult(vkAllocateDescriptorSets(vk.device, &allocInfo, &vk.worldDrawSet), "vkAllocateDescriptorSets(world draws)"))
		{
			return;
		}

		Com_Memset(&scenePush, 0, sizeof(scenePush));
		scenePush.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		scenePush.size       = VK_SCENE_PUSH_FLOATS * sizeof(float);

		setLayouts[0]                     = vk.bindlessSetLayout;
		setLayouts[1]                     = vk.worldDrawSetLayout;
		layoutInfo.setLayoutCount         = 2;
		layoutInfo.pSetLayouts            = setLayouts;
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges    = &scenePush;
		if (!VK_CheckResult(vkCreatePipelineLayout(vk.device, &layoutInfo, NULL, &vk.sceneBindlessPipelineLayout), "vkCreatePipelineLayout(world batches)"))
		{
			return;
		}
	}

	vk.sceneStaticBLVertModule    = VK_CreateShaderModule(tr3d_static_bl_vert_spv, tr3d_static_bl_vert_spv_SIZE);
	vk.sceneLMStaticBLVertModule  = VK_CreateShaderModule(tr3d_lm_static_bl_vert_spv, tr3d_lm_static_bl_vert_spv_SIZE);
	vk.sceneBLFragModule          = VK_CreateShaderModule(tr3d_bl_frag_spv, tr3d_bl_frag_spv_SIZE);
	vk.sceneLMBLFragModule        = VK_CreateShaderModule(tr3d_lm_bl_frag_spv, tr3d_lm_bl_frag_spv_SIZE);
	vk.sceneStaticFogBLVertModule = VK_CreateShaderModule(tr3d_static_fog_bl_vert_spv, tr3d_static_fog_bl_vert_spv_SIZE);
}

static void VK_DestroyCullBuffer(VkBuffer *buffer, VkDeviceMemory *memory)
{
	if (*buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(vk.device, *buffer, NULL);
		*buffer = VK_NULL_HANDLE;
	}
	if (*memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(vk.device, *memory, NULL);
		*memory = VK_NULL_HANDLE;
	}
}

/**
 * @brief Destroys the map's GPU world culling buffers (waits for the GPU,
 * only called at map load and shutdown).
 */
static void VK_DestroyWorldCull(void)
{
	if (vk.cullSurfBuffer == VK_NULL_HANDLE && vk.cullArenaBuffer == VK_NULL_HANDLE && vk.cullOutBuffer == VK_NULL_HANDLE &&
	    vk.cullGatherBuffer == VK_NULL_HANDLE && vk.cullBatchCmdBuffer == VK_NULL_HANDLE && vk.cullWorldDrawBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	vkDeviceWaitIdle(vk.device);

	if (vk.cullArenaMapped)
	{
		vkUnmapMemory(vk.device, vk.cullArenaMemory);
		vk.cullArenaMapped = NULL;
	}
	VK_DestroyCullBuffer(&vk.cullSurfBuffer, &vk.cullSurfMemory);
	VK_DestroyCullBuffer(&vk.cullLeafBuffer, &vk.cullLeafMemory);
	VK_DestroyCullBuffer(&vk.cullIndexBuffer, &vk.cullIndexMemory);
	VK_DestroyCullBuffer(&vk.cullArenaBuffer, &vk.cullArenaMemory);
	VK_DestroyCullBuffer(&vk.cullOutBuffer, &vk.cullOutMemory);
	if (vk.cullGatherMapped)
	{
		vkUnmapMemory(vk.device, vk.cullGatherMemory);
		vk.cullGatherMapped = NULL;
	}
	if (vk.cullWorldDrawMapped)
	{
		vkUnmapMemory(vk.device, vk.cullWorldDrawMemory);
		vk.cullWorldDrawMapped = NULL;
	}
	VK_DestroyCullBuffer(&vk.cullGatherBuffer, &vk.cullGatherMemory);
	VK_DestroyCullBuffer(&vk.cullBatchCmdBuffer, &vk.cullBatchCmdMemory);
	VK_DestroyCullBuffer(&vk.cullWorldDrawBuffer, &vk.cullWorldDrawMemory);
	vk.cullMaxBatchDraws = 0;
	vk.cullNumSurfs      = vk.cullNumGroups = vk.cullNumIndexes = 0;
}

/**
 * @brief Creates a host-visible buffer and copies data into it (bytes may
 * exceed the data: the rest is zeroed); mapped (left mapped if requested).
 */
static qboolean VK_CreateCullBuffer(VkDeviceSize bytes, VkBufferUsageFlags usage, const void *data, VkDeviceSize dataBytes,
                                    VkBuffer *buffer, VkDeviceMemory *memory, void **keepMapped)
{
	void *mapped;

	if (!VK_CreateBuffer(bytes, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, memory) ||
	    !VK_CheckResult(vkMapMemory(vk.device, *memory, 0, VK_WHOLE_SIZE, 0, &mapped), "vkMapMemory(cull)"))
	{
		return qfalse;
	}

	Com_Memset(mapped, 0, (size_t)bytes);
	if (data)
	{
		Com_Memcpy(mapped, data, (size_t)dataBytes);
	}

	if (keepMapped)
	{
		*keepMapped = mapped;
	}
	else
	{
		vkUnmapMemory(vk.device, *memory);
	}

	return qtrue;
}

/**
 * @brief Replaces the GPU world culling data with the loaded map's (see
 * R_VkBuildWorldCull): the culled surfaces, their leaves (indexes of the
 * world's nodes array), their triangles (indexes into the static world
 * buffer, numIndexes in all - also the size of one view's output, which
 * holds every group's region) and the number of leaves and groups.
 * @return qfalse if GPU culling is not available (the CPU walks the world)
 */
qboolean VK_CreateWorldCull(const vkCullSurface_t *surfs, int numSurfs, const unsigned int *surfLeaves, int numSurfLeaves,
                            const unsigned int *indexes, int numIndexes, int numLeaves, int numGroups)
{
	VkDescriptorBufferInfo infos[7];
	VkWriteDescriptorSet   writes[7];
	VkDeviceSize           outBytes;
	void                   *arena;
	int                    i;

	if (!vk.initialized)
	{
		return qfalse;
	}

	VK_DestroyWorldCull();

	if (vk.cullPipeline == VK_NULL_HANDLE || numSurfs <= 0 || numSurfLeaves <= 0 || numIndexes <= 0 || numGroups <= 0)
	{
		return qfalse;
	}

	vk.cullLeafWords = (unsigned int)(numLeaves + 31) / 32;
	vk.cullViewWords = vk.cullLeafWords + (unsigned int)numGroups * 5;
	vk.cullSlotWords = (unsigned int)numSurfs + VK_CULL_MAX_VIEWS * vk.cullViewWords;
	outBytes         = (VkDeviceSize)VK_FRAMES_IN_FLIGHT * VK_CULL_MAX_VIEWS * numIndexes * sizeof(unsigned int);

	if (!VK_CreateCullBuffer((VkDeviceSize)numSurfs * sizeof(vkCullSurface_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                         surfs, (VkDeviceSize)numSurfs * sizeof(vkCullSurface_t), &vk.cullSurfBuffer, &vk.cullSurfMemory, NULL) ||
	    !VK_CreateCullBuffer((VkDeviceSize)numSurfLeaves * sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                         surfLeaves, (VkDeviceSize)numSurfLeaves * sizeof(unsigned int), &vk.cullLeafBuffer, &vk.cullLeafMemory, NULL) ||
	    !VK_CreateCullBuffer((VkDeviceSize)numIndexes * sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                         indexes, (VkDeviceSize)numIndexes * sizeof(unsigned int), &vk.cullIndexBuffer, &vk.cullIndexMemory, NULL) ||
	    !VK_CreateCullBuffer((VkDeviceSize)VK_FRAMES_IN_FLIGHT * vk.cullSlotWords * sizeof(unsigned int),
	                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
	                         NULL, 0, &vk.cullArenaBuffer, &vk.cullArenaMemory, &arena) ||
	    !VK_CreateBuffer(outBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vk.cullOutBuffer, &vk.cullOutMemory))
	{
		VK_DestroyWorldCull();
		return qfalse;
	}
	vk.cullArenaMapped = (unsigned int *)arena;

	// merged world batches (optional: without them every group draws alone)
	if (vk.gatherPipeline != VK_NULL_HANDLE && vk.sceneBindlessPipelineLayout != VK_NULL_HANDLE)
	{
		void *gather, *draws;
		int  maxDraws = VK_CULL_MAX_VIEWS * numGroups * MAX_SHADER_STAGES;

		if (VK_CreateCullBuffer((VkDeviceSize)VK_FRAMES_IN_FLIGHT * maxDraws * sizeof(unsigned int), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		                        NULL, 0, &vk.cullGatherBuffer, &vk.cullGatherMemory, &gather) &&
		    VK_CreateCullBuffer((VkDeviceSize)VK_FRAMES_IN_FLIGHT * maxDraws * sizeof(vkWorldDraw_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		                        NULL, 0, &vk.cullWorldDrawBuffer, &vk.cullWorldDrawMemory, &draws) &&
		    VK_CreateBuffer((VkDeviceSize)VK_FRAMES_IN_FLIGHT * maxDraws * 5 * sizeof(unsigned int),
		                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vk.cullBatchCmdBuffer, &vk.cullBatchCmdMemory))
		{
			VkDescriptorBufferInfo drawInfo;
			VkWriteDescriptorSet   drawWrite;

			vk.cullGatherMapped    = (unsigned int *)gather;
			vk.cullWorldDrawMapped = (vkWorldDraw_t *)draws;
			vk.cullMaxBatchDraws   = maxDraws;

			Com_Memset(&drawInfo, 0, sizeof(drawInfo));
			drawInfo.buffer = vk.cullWorldDrawBuffer;
			drawInfo.range  = VK_WHOLE_SIZE;

			Com_Memset(&drawWrite, 0, sizeof(drawWrite));
			drawWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			drawWrite.dstSet          = vk.worldDrawSet;
			drawWrite.dstBinding      = 0;
			drawWrite.descriptorCount = 1;
			drawWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			drawWrite.pBufferInfo     = &drawInfo;
			vkUpdateDescriptorSets(vk.device, 1, &drawWrite, 0, NULL);
		}
	}

	Com_Memset(infos, 0, sizeof(infos));
	infos[0].buffer = vk.cullSurfBuffer;
	infos[1].buffer = vk.cullLeafBuffer;
	infos[2].buffer = vk.cullIndexBuffer;
	infos[3].buffer = vk.cullArenaBuffer;
	infos[4].buffer = vk.cullOutBuffer;
	infos[5].buffer = vk.cullGatherBuffer;
	infos[6].buffer = vk.cullBatchCmdBuffer;

	Com_Memset(writes, 0, sizeof(writes));
	for (i = 0; i < (vk.cullMaxBatchDraws ? 7 : 5); i++)
	{
		infos[i].range = VK_WHOLE_SIZE;

		writes[i].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[i].dstSet          = vk.cullSet;
		writes[i].dstBinding      = i;
		writes[i].descriptorCount = 1;
		writes[i].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[i].pBufferInfo     = &infos[i];
	}
	vkUpdateDescriptorSets(vk.device, vk.cullMaxBatchDraws ? 7 : 5, writes, 0, NULL);

	vk.cullNumSurfs   = numSurfs;
	vk.cullNumGroups  = numGroups;
	vk.cullNumIndexes = numIndexes;

	Ren_Developer("...GPU world culling: %i surfaces, %i groups, %i indexes\n", numSurfs, numGroups, numIndexes);
	return qtrue;
}

/**
 * @brief Starts GPU culling for a world view: reserves the view's slice of
 * this frame's arena, clears the caller's visible-leaf bitset
 * (cv->leafBits, VK_CullLeafWords() words) and writes the view's draw
 * commands (group g draws from groupFirst[g] in the view's output region,
 * count 0 until the dispatch adds the visible surfaces).
 * @return qfalse if the view must walk the world on the CPU (no frame being
 * recorded, no GPU culling for this map, or too many views this frame)
 */
qboolean VK_CullBeginView(vkCullView_t *cv, const int *groupFirst)
{
	unsigned int *slot, *cmd;
	unsigned int outBase;
	int          g;

	if (!vk.initialized || !vk.inFrame || !vk.cullArenaMapped || vk.cullNumViews >= VK_CULL_MAX_VIEWS)
	{
		return qfalse;
	}

	if (++vk.cullStamp == 0)
	{
		vk.cullStamp = 1; // 0 is the arena's initial skip stamp
	}

	cv->view  = vk.cullNumViews++;
	cv->stamp = vk.cullStamp;
	slot      = vk.cullArenaMapped + (size_t)vk.frameIndex * vk.cullSlotWords;
	cv->skip  = slot;

	// the caller's leaf bitset is in CPU memory: setting bits one by one in
	// mapped GPU memory reads it back (uncached), VK_CullDispatch copies it
	Com_Memset(cv->leafBits, 0, vk.cullLeafWords * sizeof(unsigned int));

	outBase = (unsigned int)((vk.frameIndex * VK_CULL_MAX_VIEWS + cv->view) * vk.cullNumIndexes);
	cmd     = slot + vk.cullNumSurfs + cv->view * vk.cullViewWords + vk.cullLeafWords;
	for (g = 0; g < vk.cullNumGroups; g++, cmd += 5)
	{
		cmd[0] = 0;
		cmd[1] = 1;
		cmd[2] = outBase + (unsigned int)groupFirst[g];
		cmd[3] = 0;
		cmd[4] = 0;
	}

	return qtrue;
}

/**
 * @brief Records a world view's culling dispatch (VK_CullBeginView) into
 * this frame's cull command buffer, which VK_EndFrame() submits before the
 * frame's draws. frustum/viewOrigin as R_CullSurface() uses them; flags:
 * VK_CULL_NOCULL, VK_CULL_FACEPLANE.
 */
void VK_CullDispatch(const vkCullView_t *cv, const cplane_t *frustum, const vec3_t viewOrigin, int flags)
{
	VkCommandBuffer cb = vk.cullCommandBuffers[vk.frameIndex];
	vkCullPush_t    push;
	unsigned int    slotOfs = (unsigned int)vk.frameIndex * vk.cullSlotWords;
	int             i;

	if (!vk.cullRecording)
	{
		VkCommandBufferBeginInfo beginInfo;

		vkResetCommandBuffer(cb, 0);
		Com_Memset(&beginInfo, 0, sizeof(beginInfo));
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cb, &beginInfo);
		vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, vk.cullPipeline);
		vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE, vk.cullPipelineLayout, 0, 1, &vk.cullSet, 0, NULL);
		vk.cullRecording = qtrue;
	}

	Com_Memset(&push, 0, sizeof(push));
	for (i = 0; i < 5; i++)
	{
		push.frustum[i][0] = frustum[i].normal[0];
		push.frustum[i][1] = frustum[i].normal[1];
		push.frustum[i][2] = frustum[i].normal[2];
		push.frustum[i][3] = frustum[i].dist;
	}
	push.viewOrigin[0] = viewOrigin[0];
	push.viewOrigin[1] = viewOrigin[1];
	push.viewOrigin[2] = viewOrigin[2];
	push.numSurfs      = (unsigned int)vk.cullNumSurfs;
	push.leafBitsOfs   = slotOfs + (unsigned int)vk.cullNumSurfs + (unsigned int)cv->view * vk.cullViewWords;
	push.skipOfs       = slotOfs;
	push.cmdOfs        = push.leafBitsOfs + vk.cullLeafWords;
	push.viewStamp     = cv->stamp;
	push.flags         = (unsigned int)flags;

	Com_Memcpy(vk.cullArenaMapped + push.leafBitsOfs, cv->leafBits, vk.cullLeafWords * sizeof(unsigned int));

	vkCmdPushConstants(cb, vk.cullPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
	vkCmdDispatch(cb, (uint32_t)(vk.cullNumSurfs + 63) / 64, 1, 1);
}

/**
 * @brief Size of a view's visible-leaf bitset (32-bit words)
 */
unsigned int VK_CullLeafWords(void)
{
	return vk.cullLeafWords;
}

/**
 * @brief Byte offset in the cull arena of a view's draw command for a group
 * (for VK_Draw3DStatic's indirect draw, tess.vkIndirectOffset).
 */
unsigned int VK_CullCommandOffset(int view, int group)
{
	return (unsigned int)(((size_t)vk.frameIndex * vk.cullSlotWords + vk.cullNumSurfs + (size_t)view * vk.cullViewWords +
	                       vk.cullLeafWords + (size_t)group * 5) * sizeof(unsigned int));
}

/**
 * @brief Creates the two pipeline layouts used by static 3D
 * geometry draws - one with a single descriptor set (base texture only,
 * used by RB_StageIteratorGeneric/VertexLitTexture) and one with two
 * descriptor sets (base + lightmap, used by RB_StageIteratorLightmappedMultitexture
 * and 2-stage shaders in the generic path). Both reuse
 * vk.uiDescriptorSetLayout (single combined-image-sampler) since every
 * image already gets one such descriptor set from VK_CreateImageTexture.
 * Also pre-compiles the shader modules shared by every lazily-created
 * pipeline variant.
 */
static qboolean VK_CreateScenePipelineLayouts(void)
{
	VkPushConstantRange        pushConstantRange;
	VkPipelineLayoutCreateInfo layoutInfo;
	VkDescriptorSetLayout      twoSetLayouts[2];

	Com_Memset(&pushConstantRange, 0, sizeof(pushConstantRange));
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset     = 0;
	pushConstantRange.size       = sizeof(float) * VK_SCENE_PUSH_FLOATS;

	Com_Memset(&layoutInfo, 0, sizeof(layoutInfo));
	layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount         = 1;
	layoutInfo.pSetLayouts            = &vk.uiDescriptorSetLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges    = &pushConstantRange;

	if (!VK_CheckResult(vkCreatePipelineLayout(vk.device, &layoutInfo, NULL, &vk.scenePipelineLayout), "vkCreatePipelineLayout(scene)"))
	{
		return qfalse;
	}

	twoSetLayouts[0] = vk.uiDescriptorSetLayout;
	twoSetLayouts[1] = vk.uiDescriptorSetLayout;

	layoutInfo.setLayoutCount = 2;
	layoutInfo.pSetLayouts    = twoSetLayouts;

	if (!VK_CheckResult(vkCreatePipelineLayout(vk.device, &layoutInfo, NULL, &vk.sceneLMPipelineLayout), "vkCreatePipelineLayout(sceneLM)"))
	{
		return qfalse;
	}

	vk.sceneVertModule   = VK_CreateShaderModule(tr3d_vert_spv, tr3d_vert_spv_SIZE);
	vk.sceneFragModule   = VK_CreateShaderModule(tr3d_frag_spv, tr3d_frag_spv_SIZE);
	vk.sceneLMVertModule = VK_CreateShaderModule(tr3d_lm_vert_spv, tr3d_lm_vert_spv_SIZE);
	vk.sceneLMFragModule = VK_CreateShaderModule(tr3d_lm_frag_spv, tr3d_lm_frag_spv_SIZE);

	vk.sceneStaticVertModule     = VK_CreateShaderModule(tr3d_static_vert_spv, tr3d_static_vert_spv_SIZE);
	vk.sceneLMStaticVertModule   = VK_CreateShaderModule(tr3d_lm_static_vert_spv, tr3d_lm_static_vert_spv_SIZE);
	vk.sceneDiffuseVertModule    = VK_CreateShaderModule(tr3d_diffuse_vert_spv, tr3d_diffuse_vert_spv_SIZE);
	vk.sceneSkinVertModule       = VK_CreateShaderModule(tr3d_skin_vert_spv, tr3d_skin_vert_spv_SIZE);
	vk.sceneMD3VertModule        = VK_CreateShaderModule(tr3d_md3_vert_spv, tr3d_md3_vert_spv_SIZE);
	vk.sceneFoliageVertModule    = VK_CreateShaderModule(tr3d_foliage_vert_spv, tr3d_foliage_vert_spv_SIZE);
	vk.sceneFoliageFogVertModule = VK_CreateShaderModule(tr3d_foliage_fog_vert_spv, tr3d_foliage_fog_vert_spv_SIZE);

	vk.sceneLMStaticDlightVertModule = VK_CreateShaderModule(tr3d_lm_static_dlight_vert_spv, tr3d_lm_static_dlight_vert_spv_SIZE);
	vk.sceneLMDlightFragModule       = VK_CreateShaderModule(tr3d_lm_dlight_frag_spv, tr3d_lm_dlight_frag_spv_SIZE);
	vk.scene2StageVertModule         = VK_CreateShaderModule(tr3d_2stage_vert_spv, tr3d_2stage_vert_spv_SIZE);
	vk.scene2StageFragModule         = VK_CreateShaderModule(tr3d_2stage_frag_spv, tr3d_2stage_frag_spv_SIZE);

	if (vk.sceneVertModule == VK_NULL_HANDLE || vk.sceneFragModule == VK_NULL_HANDLE ||
	    vk.sceneLMVertModule == VK_NULL_HANDLE || vk.sceneLMFragModule == VK_NULL_HANDLE ||
	    vk.sceneStaticVertModule == VK_NULL_HANDLE || vk.sceneLMStaticVertModule == VK_NULL_HANDLE ||
	    vk.sceneDiffuseVertModule == VK_NULL_HANDLE || vk.sceneSkinVertModule == VK_NULL_HANDLE ||
	    vk.sceneLMStaticDlightVertModule == VK_NULL_HANDLE || vk.sceneLMDlightFragModule == VK_NULL_HANDLE ||
	    vk.scene2StageVertModule == VK_NULL_HANDLE || vk.scene2StageFragModule == VK_NULL_HANDLE ||
	    vk.sceneMD3VertModule == VK_NULL_HANDLE || vk.sceneFoliageVertModule == VK_NULL_HANDLE ||
	    vk.sceneFoliageFogVertModule == VK_NULL_HANDLE)
	{
		return qfalse;
	}

	// GPU skinning: set 1 = the bone arena as a dynamic uniform buffer
	{
		VkDescriptorSetLayoutBinding    binding;
		VkDescriptorSetLayoutCreateInfo setLayoutInfo;

		Com_Memset(&binding, 0, sizeof(binding));
		binding.binding         = 0;
		binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		binding.descriptorCount = 1;
		binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

		Com_Memset(&setLayoutInfo, 0, sizeof(setLayoutInfo));
		setLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setLayoutInfo.bindingCount = 1;
		setLayoutInfo.pBindings    = &binding;

		if (!VK_CheckResult(vkCreateDescriptorSetLayout(vk.device, &setLayoutInfo, NULL, &vk.boneSetLayout), "vkCreateDescriptorSetLayout(bones)"))
		{
			return qfalse;
		}

		// texture, bones (or mesh normal tables), env params: the same
		// dynamic uniform set at a second offset
		{
			VkDescriptorSetLayout threeSetLayouts[3];

			threeSetLayouts[0] = vk.uiDescriptorSetLayout;
			threeSetLayouts[1] = vk.boneSetLayout;
			threeSetLayouts[2] = vk.boneSetLayout;

			layoutInfo.setLayoutCount = 3;
			layoutInfo.pSetLayouts    = threeSetLayouts;

			if (!VK_CheckResult(vkCreatePipelineLayout(vk.device, &layoutInfo, NULL, &vk.skinPipelineLayout), "vkCreatePipelineLayout(skin)"))
			{
				return qfalse;
			}
		}

		// folded dlights: base, lightmap, then the same dynamic uniform set
		{
			VkDescriptorSetLayout threeSetLayouts[3];

			threeSetLayouts[0] = vk.uiDescriptorSetLayout;
			threeSetLayouts[1] = vk.uiDescriptorSetLayout;
			threeSetLayouts[2] = vk.boneSetLayout;

			layoutInfo.setLayoutCount = 3;
			layoutInfo.pSetLayouts    = threeSetLayouts;

			if (!VK_CheckResult(vkCreatePipelineLayout(vk.device, &layoutInfo, NULL, &vk.staticDlightPipelineLayout), "vkCreatePipelineLayout(static dlight)"))
			{
				return qfalse;
			}
		}
	}

	return qtrue;
}

/**
 * @brief Creates one 3D scene pipeline variant for the given (stateBits,
 * cullType) pair. `lightmapped` selects the vertex format/shaders/pipeline
 * layout (vk3DVertex_t + tr3d.* vs. vk3DVertexLM_t + tr3d_lm.*).
 */
static VkPipeline VK_CreateScenePipelineVariant(unsigned long stateBits, int cullType, qboolean lightmapped, qboolean polygonOffset, int special, int vertexSource)
{
	VkPipelineShaderStageCreateInfo        stages[2];
	VkVertexInputBindingDescription        vertexBindings[5];
	uint32_t                               bindingCount = 1;
	VkVertexInputAttributeDescription      vertexAttributes[8];
	uint32_t                               attributeCount;
	VkPipelineVertexInputStateCreateInfo   vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkPipelineViewportStateCreateInfo      viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo   multisampling;
	VkPipelineDepthStencilStateCreateInfo  depthStencil;
	VkPipelineColorBlendAttachmentState    colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo    colorBlending;
	VkDynamicState                         dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo       dynamicState;
	VkGraphicsPipelineCreateInfo           pipelineInfo;
	VkPipeline                             pipeline = VK_NULL_HANDLE;
	qboolean                               cullFront;
	uint32_t                               ai;

	Com_Memset(stages, 0, sizeof(stages));
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	if (vertexSource == VK_VERTS_DIFFUSE)
	{
		stages[0].module = vk.sceneDiffuseVertModule;
	}
	else if (vertexSource == VK_VERTS_SKIN)
	{
		stages[0].module = vk.sceneSkinVertModule;
	}
	else if (vertexSource == VK_VERTS_SKIN_BL)
	{
		stages[0].module = vk.sceneSkinBLVertModule;
	}
	else if (vertexSource == VK_VERTS_MD3)
	{
		stages[0].module = vk.sceneMD3VertModule;
	}
	else if (vertexSource == VK_VERTS_STATIC_DLIGHT)
	{
		stages[0].module = vk.sceneLMStaticDlightVertModule;
	}
	else if (vertexSource == VK_VERTS_TWOSTAGE)
	{
		stages[0].module = vk.scene2StageVertModule;
	}
	else if (vertexSource == VK_VERTS_STATIC_BL || vertexSource == VK_VERTS_STATIC_LIGHTMAP_BL)
	{
		stages[0].module = lightmapped ? vk.sceneLMStaticBLVertModule : vk.sceneStaticBLVertModule;
	}
	else if (vertexSource == VK_VERTS_MD3_BL)
	{
		stages[0].module = vk.sceneMD3BLVertModule;
	}
	else if (vertexSource == VK_VERTS_STATIC_FOG_BL)
	{
		stages[0].module = vk.sceneStaticFogBLVertModule;
	}
	else if (vertexSource == VK_VERTS_FOLIAGE || vertexSource == VK_VERTS_FOLIAGE_FOG)
	{
		stages[0].module = (vertexSource == VK_VERTS_FOLIAGE) ? vk.sceneFoliageVertModule : vk.sceneFoliageFogVertModule;
	}
	else if (vertexSource != VK_VERTS_ARENA)
	{
		stages[0].module = lightmapped ? vk.sceneLMStaticVertModule : vk.sceneStaticVertModule;
	}
	else
	{
		stages[0].module = lightmapped ? vk.sceneLMVertModule : vk.sceneVertModule;
	}
	stages[0].pName  = "main";
	stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = (vertexSource == VK_VERTS_STATIC_DLIGHT) ? vk.sceneLMDlightFragModule :
	                   (vertexSource == VK_VERTS_TWOSTAGE) ? vk.scene2StageFragModule :
	                   (vertexSource == VK_VERTS_STATIC_BL || vertexSource == VK_VERTS_STATIC_LIGHTMAP_BL) ?
	                   (lightmapped ? vk.sceneLMBLFragModule : vk.sceneBLFragModule) :
	                   (vertexSource == VK_VERTS_MD3_BL || vertexSource == VK_VERTS_SKIN_BL || vertexSource == VK_VERTS_STATIC_FOG_BL) ?
	                   vk.sceneBLFragModule :
	                   (lightmapped ? vk.sceneLMFragModule : vk.sceneFragModule);
	stages[1].pName = "main";

	Com_Memset(vertexBindings, 0, sizeof(vertexBindings));
	vertexBindings[0].binding   = 0;
	vertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	Com_Memset(vertexAttributes, 0, sizeof(vertexAttributes));
	if (vertexSource == VK_VERTS_MD3_BL)
	{
		// no vertex streams: tr3d_md3_bl.vert reads the mesh buffer
		bindingCount   = 0;
		attributeCount = 0;
	}
	else if (vertexSource == VK_VERTS_MD3)
	{
		// binding 0: the entity's frame, 1: its oldframe (md3XyzNormal_t:
		// short xyz[3], short latlong normal; MDC: base frames), 2: texture
		// coords, 3/4: MDC compressed frame/oldframe (mdcXyzCompressed_t)
		static const VkFormat md3Formats[5] =
		{
			VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32_UINT, VK_FORMAT_R32_UINT
		};
		static const uint32_t md3Strides[5] =
		{
			sizeof(md3XyzNormal_t), sizeof(md3XyzNormal_t), sizeof(md3St_t), sizeof(mdcXyzCompressed_t), sizeof(mdcXyzCompressed_t)
		};

		for (ai = 0; ai < 5; ai++)
		{
			vertexBindings[ai].binding   = ai;
			vertexBindings[ai].stride    = md3Strides[ai];
			vertexBindings[ai].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			vertexAttributes[ai].location = ai;
			vertexAttributes[ai].binding  = ai;
			vertexAttributes[ai].format   = md3Formats[ai];
			vertexAttributes[ai].offset   = 0;
		}
		bindingCount   = 5;
		attributeCount = 5;
	}
	else if (vertexSource == VK_VERTS_FOLIAGE || vertexSource == VK_VERTS_FOLIAGE_FOG)
	{
		// binding 0: the model in the static world buffer, 1: one
		// foliageInstance_t per instance (origin, color)
		vertexBindings[0].stride    = sizeof(vk3DVertexLM_t);
		vertexBindings[1].binding   = 1;
		vertexBindings[1].stride    = sizeof(foliageInstance_t);
		vertexBindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		vertexAttributes[0].location = 0;
		vertexAttributes[0].binding  = 0;
		vertexAttributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributes[0].offset   = offsetof(vk3DVertexLM_t, pos);
		vertexAttributes[1].location = 1;
		vertexAttributes[1].binding  = 0;
		vertexAttributes[1].format   = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[1].offset   = offsetof(vk3DVertexLM_t, uv0);
		vertexAttributes[2].location = 2;
		vertexAttributes[2].binding  = 1;
		vertexAttributes[2].format   = VK_FORMAT_R8G8B8A8_UNORM;
		vertexAttributes[2].offset   = offsetof(foliageInstance_t, color);
		vertexAttributes[3].location = 3;
		vertexAttributes[3].binding  = 1;
		vertexAttributes[3].format   = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributes[3].offset   = offsetof(foliageInstance_t, origin);
		bindingCount                 = 2;
		attributeCount               = 4;
	}
	else if (vertexSource == VK_VERTS_TWOSTAGE)
	{
		vertexBindings[0].stride = sizeof(vk3DVertex2_t);

		vertexAttributes[0].location = 0;
		vertexAttributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributes[0].offset   = offsetof(vk3DVertex2_t, pos);
		vertexAttributes[1].location = 1;
		vertexAttributes[1].format   = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[1].offset   = offsetof(vk3DVertex2_t, uv0);
		vertexAttributes[2].location = 2;
		vertexAttributes[2].format   = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[2].offset   = offsetof(vk3DVertex2_t, uv1);
		vertexAttributes[3].location = 3;
		vertexAttributes[3].format   = VK_FORMAT_R8G8B8A8_UNORM;
		vertexAttributes[3].offset   = offsetof(vk3DVertex2_t, color0);
		vertexAttributes[4].location = 4;
		vertexAttributes[4].format   = VK_FORMAT_R8G8B8A8_UNORM;
		vertexAttributes[4].offset   = offsetof(vk3DVertex2_t, color1);
		attributeCount               = 5;
	}
	else if (vertexSource == VK_VERTS_SKIN || vertexSource == VK_VERTS_SKIN_BL)
	{
		vertexBindings[0].stride = sizeof(vkSkinVertex_t);

		vertexAttributes[0].location = 0;
		vertexAttributes[0].format   = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[0].offset   = offsetof(vkSkinVertex_t, uv);
		vertexAttributes[1].location = 1;
		vertexAttributes[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributes[1].offset   = offsetof(vkSkinVertex_t, normal);
		vertexAttributes[2].location = 2;
		vertexAttributes[2].format   = VK_FORMAT_R8G8B8A8_UINT;
		vertexAttributes[2].offset   = offsetof(vkSkinVertex_t, bones);
		vertexAttributes[3].location = 3;
		vertexAttributes[3].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertexAttributes[3].offset   = offsetof(vkSkinVertex_t, weights);
		for (ai = 0; ai < 4; ai++)
		{
			vertexAttributes[4 + ai].location = 4 + ai;
			vertexAttributes[4 + ai].format   = VK_FORMAT_R32G32B32_SFLOAT;
			vertexAttributes[4 + ai].offset   = offsetof(vkSkinVertex_t, offsets) + ai * 3 * sizeof(float);
		}
		attributeCount = 8;
	}
	else if (vertexSource == VK_VERTS_DIFFUSE)
	{
		vertexBindings[0].stride = sizeof(vk3DVertexN_t);

		vertexAttributes[0].location = 0;
		vertexAttributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributes[0].offset   = offsetof(vk3DVertexN_t, pos);
		vertexAttributes[1].location = 1;
		vertexAttributes[1].format   = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[1].offset   = offsetof(vk3DVertexN_t, uv);
		vertexAttributes[2].location = 2;
		vertexAttributes[2].format   = VK_FORMAT_R8G8B8A8_UNORM;
		vertexAttributes[2].offset   = offsetof(vk3DVertexN_t, color);
		vertexAttributes[3].location = 3;
		vertexAttributes[3].format   = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributes[3].offset   = offsetof(vk3DVertexN_t, normal);
		attributeCount               = 4;
	}
	else if (vertexSource != VK_VERTS_ARENA && !lightmapped)
	{
		// static world buffer read as single-texture vertices
		vertexBindings[0].stride = sizeof(vk3DVertexLM_t);

		vertexAttributes[0].location = 0;
		vertexAttributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributes[0].offset   = offsetof(vk3DVertexLM_t, pos);
		vertexAttributes[1].location = 1;
		vertexAttributes[1].format   = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[1].offset   = (vertexSource == VK_VERTS_STATIC_LIGHTMAP || vertexSource == VK_VERTS_STATIC_LIGHTMAP_BL) ?
		                               offsetof(vk3DVertexLM_t, uv1) : offsetof(vk3DVertexLM_t, uv0);
		vertexAttributes[2].location = 2;
		vertexAttributes[2].format   = VK_FORMAT_R8G8B8A8_UNORM;
		vertexAttributes[2].offset   = offsetof(vk3DVertexLM_t, color);
		attributeCount               = 3;
	}
	else if (lightmapped)
	{
		vertexBindings[0].stride = sizeof(vk3DVertexLM_t);

		vertexAttributes[0].location = 0;
		vertexAttributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributes[0].offset   = offsetof(vk3DVertexLM_t, pos);
		vertexAttributes[1].location = 1;
		vertexAttributes[1].format   = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[1].offset   = offsetof(vk3DVertexLM_t, uv0);
		vertexAttributes[2].location = 2;
		vertexAttributes[2].format   = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[2].offset   = offsetof(vk3DVertexLM_t, uv1);
		vertexAttributes[3].location = 3;
		vertexAttributes[3].format   = VK_FORMAT_R8G8B8A8_UNORM;
		vertexAttributes[3].offset   = offsetof(vk3DVertexLM_t, color);
		attributeCount               = 4;
	}
	else
	{
		vertexBindings[0].stride = sizeof(vk3DVertex_t);

		vertexAttributes[0].location = 0;
		vertexAttributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributes[0].offset   = offsetof(vk3DVertex_t, pos);
		vertexAttributes[1].location = 1;
		vertexAttributes[1].format   = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributes[1].offset   = offsetof(vk3DVertex_t, uv);
		vertexAttributes[2].location = 2;
		vertexAttributes[2].format   = VK_FORMAT_R8G8B8A8_UNORM;
		vertexAttributes[2].offset   = offsetof(vk3DVertex_t, color);
		attributeCount               = 3;
	}
	for (ai = 0; ai < attributeCount && bindingCount == 1; ai++)
	{
		vertexAttributes[ai].binding = 0;
	}

	Com_Memset(&vertexInputInfo, 0, sizeof(vertexInputInfo));
	vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount   = bindingCount;
	vertexInputInfo.pVertexBindingDescriptions      = vertexBindings;
	vertexInputInfo.vertexAttributeDescriptionCount = attributeCount;
	vertexInputInfo.pVertexAttributeDescriptions    = vertexAttributes;

	Com_Memset(&inputAssembly, 0, sizeof(inputAssembly));
	inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = (special == VK_SPECIAL_LINES) ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	Com_Memset(&viewportState, 0, sizeof(viewportState));
	viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount  = 1;

	// GL_Cull: cullFront = (cullType == CT_FRONT_SIDED); glCullFace(cullFront ? GL_FRONT : GL_BACK).
	// The Vulkan clip-space Y-flip baked into VK_SetProjectionMatrix cancels
	// out against Vulkan's inherently Y-down viewport mapping, so winding
	// order as seen by the rasterizer matches GL's - no frontFace flip needed.
	cullFront = (cullType == CT_FRONT_SIDED);

	Com_Memset(&rasterizer, 0, sizeof(rasterizer));
	rasterizer.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode    = cullType == CT_TWO_SIDED ? VK_CULL_MODE_NONE : (cullFront ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_BACK_BIT);
	rasterizer.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.lineWidth   = 1.0f;

	// shader->polygonOffset (decals, marks): renderer1 enables
	// GL_POLYGON_OFFSET_FILL with glPolygonOffset(r_offsetFactor,
	// r_offsetUnits). Baked into the pipeline; both cvars are CVAR_CHEAT
	// and effectively never change at runtime.
	if (polygonOffset)
	{
		rasterizer.depthBiasEnable         = VK_TRUE;
		rasterizer.depthBiasSlopeFactor    = r_offsetFactor->value;
		rasterizer.depthBiasConstantFactor = r_offsetUnits->value;
	}

	Com_Memset(&multisampling, 0, sizeof(multisampling));
	multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = vk.sampleCount; // MSAA

	Com_Memset(&depthStencil, 0, sizeof(depthStencil));
	depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable  = (stateBits & GLS_DEPTHTEST_DISABLE) ? VK_FALSE : VK_TRUE;
	depthStencil.depthWriteEnable = (stateBits & GLS_DEPTHMASK_TRUE) ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp   = (stateBits & GLS_DEPTHFUNC_EQUAL) ? VK_COMPARE_OP_EQUAL : VK_COMPARE_OP_LESS_OR_EQUAL;

	Com_Memset(&colorBlendAttachment, 0, sizeof(colorBlendAttachment));
	colorBlendAttachment.blendEnable         = (stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) ? VK_TRUE : VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_SrcBlendFactor(stateBits);
	colorBlendAttachment.dstColorBlendFactor = VK_DstBlendFactor(stateBits);
	colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_SrcBlendFactor(stateBits);
	colorBlendAttachment.dstAlphaBlendFactor = VK_DstBlendFactor(stateBits);
	colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	Com_Memset(&colorBlending, 0, sizeof(colorBlending));
	colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments    = &colorBlendAttachment;

	// Stencil shadow volume states (see VK_SPECIAL_*).
	if (special == VK_SPECIAL_SHADOW_INCR || special == VK_SPECIAL_SHADOW_DECR)
	{
		VkStencilOpState op;

		Com_Memset(&op, 0, sizeof(op));
		op.failOp      = VK_STENCIL_OP_KEEP;
		op.depthFailOp = VK_STENCIL_OP_KEEP;
		op.passOp      = (special == VK_SPECIAL_SHADOW_INCR) ? VK_STENCIL_OP_INCREMENT_AND_CLAMP : VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		op.compareOp   = VK_COMPARE_OP_ALWAYS;
		op.compareMask = 255;
		op.writeMask   = 255;
		op.reference   = 1;

		depthStencil.stencilTestEnable = VK_TRUE;
		depthStencil.front             = op;
		depthStencil.back              = op;

		colorBlendAttachment.blendEnable    = VK_FALSE;
		colorBlendAttachment.colorWriteMask = 0;
	}
	else if (special == VK_SPECIAL_SHADOW_DARKEN)
	{
		VkStencilOpState op;

		Com_Memset(&op, 0, sizeof(op));
		op.failOp      = VK_STENCIL_OP_KEEP;
		op.depthFailOp = VK_STENCIL_OP_KEEP;
		op.passOp      = VK_STENCIL_OP_KEEP;
		op.compareOp   = VK_COMPARE_OP_NOT_EQUAL;
		op.compareMask = 255;
		op.writeMask   = 0;
		op.reference   = 0;

		depthStencil.stencilTestEnable = VK_TRUE;
		depthStencil.front             = op;
		depthStencil.back              = op;
	}

	Com_Memset(&dynamicState, 0, sizeof(dynamicState));
	dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates    = dynamicStates;

	Com_Memset(&pipelineInfo, 0, sizeof(pipelineInfo));
	pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount          = 2;
	pipelineInfo.pStages             = stages;
	pipelineInfo.pVertexInputState   = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState      = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pDepthStencilState  = &depthStencil;
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.layout              = (vertexSource == VK_VERTS_SKIN || vertexSource == VK_VERTS_MD3) ? vk.skinPipelineLayout :
	                                   (vertexSource == VK_VERTS_MD3_BL || vertexSource == VK_VERTS_SKIN_BL) ? vk.sceneMD3BLPipelineLayout :
	                                   (vertexSource == VK_VERTS_STATIC_DLIGHT) ? vk.staticDlightPipelineLayout :
	                                   (vertexSource == VK_VERTS_STATIC_BL || vertexSource == VK_VERTS_STATIC_LIGHTMAP_BL ||
	                                    vertexSource == VK_VERTS_STATIC_FOG_BL) ? vk.sceneBindlessPipelineLayout :
	                                   (lightmapped ? vk.sceneLMPipelineLayout : vk.scenePipelineLayout);
	pipelineInfo.renderPass = vk.renderPass;
	pipelineInfo.subpass    = 0;

	if (!VK_CheckResult(vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline), "vkCreateGraphicsPipelines(scene)"))
	{
		return VK_NULL_HANDLE;
	}

	return pipeline;
}

/**
 * @brief Returns the cached pipeline for this (stateBits, cullType) pair,
 * creating and caching it on first use. Returns VK_NULL_HANDLE if the
 * cache is full or creation failed (caller should skip the draw).
 */
/**
 * @brief Mirrors GL_Cull()'s backEnd.viewParms.isMirror front/back flip
 * (tr_backend.c): mirrored views have inverted winding, so front/back
 * culling must be swapped to match what the real GL renderer does via
 * glCullFace(). Two-sided shaders are unaffected. Flipping cullType here
 * (rather than baking isMirror into the pipeline cache key) keeps the
 * cache correct automatically, since a flipped cullType is itself a
 * distinct, correctly-cached pipeline variant.
 */
static int VK_MirrorCullType(int cullType)
{
	if (!backEnd.viewParms.isMirror)
	{
		return cullType;
	}

	if (cullType == CT_FRONT_SIDED)
	{
		return CT_BACK_SIDED;
	}
	if (cullType == CT_BACK_SIDED)
	{
		return CT_FRONT_SIDED;
	}

	return cullType;
}

static VkPipeline VK_GetScenePipelineEx(unsigned long stateBits, int cullType, qboolean lightmapped, qboolean polygonOffset, int special, int vertexSource)
{
	vkScenePipelineEntry_t *entries = lightmapped ? vk.sceneLMPipelines : vk.scenePipelines;
	int                    *count   = lightmapped ? &vk.numSceneLMPipelines : &vk.numScenePipelines;
	int                    i;
	VkPipeline             pipeline;

	for (i = 0; i < *count; i++)
	{
		if (entries[i].stateBits == stateBits && entries[i].cullType == cullType && entries[i].polygonOffset == polygonOffset &&
		    entries[i].special == special && entries[i].vertexSource == vertexSource)
		{
			return entries[i].pipeline;
		}
	}

	if (*count >= VK_MAX_SCENE_PIPELINES)
	{
		Ren_Print("Vulkan: scene pipeline cache full, dropping draw\n");
		return VK_NULL_HANDLE;
	}

	pipeline = VK_CreateScenePipelineVariant(stateBits, cullType, lightmapped, polygonOffset, special, vertexSource);
	if (pipeline == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE;
	}

	entries[*count].stateBits     = stateBits;
	entries[*count].cullType      = cullType;
	entries[*count].polygonOffset = polygonOffset;
	entries[*count].special       = special;
	entries[*count].vertexSource  = vertexSource;
	entries[*count].pipeline      = pipeline;
	(*count)++;

	return pipeline;
}

static VkPipeline VK_GetScenePipeline(unsigned long stateBits, int cullType, qboolean lightmapped, qboolean polygonOffset, int special)
{
	return VK_GetScenePipelineEx(stateBits, cullType, lightmapped, polygonOffset, special, VK_VERTS_ARENA);
}

/**
 * @brief Multiplies two column-major 4x4 matrices (GL convention: index =
 * col*4+row), out = a * b. `out` must not alias `a` or `b`.
 */
static void VK_Mat4Mul(float *out, const float *a, const float *b)
{
	int c, r, k;

	for (c = 0; c < 4; c++)
	{
		for (r = 0; r < 4; r++)
		{
			float sum = 0.0f;

			for (k = 0; k < 4; k++)
			{
				sum += a[k * 4 + r] * b[c * 4 + k];
			}
			out[c * 4 + r] = sum;
		}
	}
}

/**
 * @brief Captures the engine's GL-convention projection matrix (called
 * from SetViewportAndScissor(), where renderer1 calls
 * glLoadMatrixf(backEnd.viewParms.projectionMatrix)) and bakes in the GL -> Vulkan
 * clip-space correction: negate the Y row (Vulkan NDC Y maps to the
 * viewport opposite of GL's, so this keeps the final image right-side up)
 * and remap Z from GL's [-1,1] range to Vulkan's [0,1] range.
 */
void VK_SetProjectionMatrix(const float *m)
{
	int c;

	for (c = 0; c < 4; c++)
	{
		vk.projMatrix[c * 4 + 0] = m[c * 4 + 0];
		vk.projMatrix[c * 4 + 1] = -m[c * 4 + 1];
		vk.projMatrix[c * 4 + 2] = 0.5f * m[c * 4 + 2] + 0.5f * m[c * 4 + 3];
		vk.projMatrix[c * 4 + 3] = m[c * 4 + 3];
	}

	VK_Mat4Mul(vk.mvpMatrix, vk.projMatrix, vk.modelMatrix);
}

/**
 * @brief Copies out the already-Vulkan-converted projection
 * matrix (used by flare rendering to save/restore the world projection
 * around its own screen-space ortho projection, mirroring renderer1's
 * glPushMatrix/glPopMatrix around glOrtho()).
 */
void VK_GetProjectionMatrix(float *m)
{
	Com_Memcpy(m, vk.projMatrix, sizeof(vk.projMatrix));
}

/**
 * @brief Sets the Vulkan-space projection matrix directly,
 * bypassing the GL->Vulkan conversion VK_SetProjectionMatrix() applies.
 * Used to restore a matrix previously captured with
 * VK_GetProjectionMatrix(), which is already in Vulkan convention.
 */
void VK_SetRawProjectionMatrix(const float *m)
{
	Com_Memcpy(vk.projMatrix, m, sizeof(vk.projMatrix));
	VK_Mat4Mul(vk.mvpMatrix, vk.projMatrix, vk.modelMatrix);
}

/**
 * @brief Captures the engine's GL-convention modelview matrix (called from
 * tr_backend.c where renderer1 calls
 * glLoadMatrixf(backEnd.orientation.modelMatrix) /
 * glLoadMatrixf(backEnd.viewParms.world.modelMatrix)) and recomputes the cached MVP used by VK_Draw3D*().
 */
void VK_SetModelViewMatrix(const float *m)
{
	Com_Memcpy(vk.modelMatrix, m, sizeof(vk.modelMatrix));
	VK_Mat4Mul(vk.mvpMatrix, vk.projMatrix, vk.modelMatrix);
}

/**
 * @brief Copies out the currently-set modelview matrix, so sky
 * rendering can temporarily zero its translation component (the classic
 * "infinite skybox stays centered on the camera" trick, matching
 * renderer1's glTranslatef(viewerOrigin) applied on top of an
 * already-loaded view matrix) and restore the original afterwards.
 */
void VK_GetModelViewMatrix(float *m)
{
	Com_Memcpy(m, vk.modelMatrix, sizeof(vk.modelMatrix));
}

/**
 * @brief Records the current fog parameters, called from
 * R_Fog() (tr_main.c) where renderer1 calls glFog*(). Linear fog uses
 * start/end; exponential (GL_EXP, density fog) uses density. Picked up
 * per-draw by the 3D draw paths and applied in the fragment shaders from
 * each fragment's eye-space depth.
 */
void VK_SetFog(const float *color, float start, float end, qboolean exponential, float density)
{
	vk.fogEnabled  = qtrue;
	vk.fogColor[0] = color[0];
	vk.fogColor[1] = color[1];
	vk.fogColor[2] = color[2];
	vk.fogColor[3] = color[3];
	vk.fogStart    = start;
	vk.fogEnd      = end;
	vk.fogExp      = exponential;
	vk.fogDensity  = density;

	vk.fogParamsValid = qtrue;
}

void VK_SetFogOff(void)
{
	vk.fogEnabled = qfalse;
}

/**
 * @brief Re-enables fog with the parameters from the last VK_SetFog(),
 * mirroring glEnable(GL_FOG) in R_FogOn() (renderer1 keeps the fog
 * parameters across glDisable/glEnable, so R_FogOff()/R_FogOn() pairs,
 * e.g. around dlight passes or per-stage noFog, must restore fog).
 */
void VK_SetFogOn(void)
{
	if (vk.fogParamsValid)
	{
		vk.fogEnabled = qtrue;
	}
}

/**
 * @brief Records an eye-space portal/mirror clip-plane
 * equation (ax+by+cz+dw, same convention as GL_CLIP_PLANE0: fragments
 * with dot(eyePos, plane.xyz) + plane.w < 0 are clipped), called from
 * RB_BeginDrawingView() (tr_backend.c) where renderer1 calls
 * glClipPlane()/glEnable(GL_CLIP_PLANE0). Applied as a per-fragment
 * discard in the scene shaders since Vulkan has no portable
 * fixed-function clip-plane equivalent.
 */
void VK_SetPortalClip(const float *planeEq)
{
	vk.clipPlaneEnabled = qtrue;
	vk.clipPlane[0]     = planeEq[0];
	vk.clipPlane[1]     = planeEq[1];
	vk.clipPlane[2]     = planeEq[2];
	vk.clipPlane[3]     = planeEq[3];
}

void VK_ClearPortalClip(void)
{
	vk.clipPlaneEnabled = qfalse;
}

/**
 * @brief Destroys everything VK_CreateRenderPassAndFramebuffers() and
 * VK_CreatePresentResources() created (render passes, framebuffers, scene
 * color/MSAA/depth targets, gamma pipeline), keeping the swapchain.
 */
static void VK_DestroySceneTargets(void)
{
	uint32_t i;

	vkDeviceWaitIdle(vk.device);

	for (i = 0; i < vk.swapchainImageCount; i++)
	{
		if (vk.swapchainFramebuffers[i] != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(vk.device, vk.swapchainFramebuffers[i], NULL);
			vk.swapchainFramebuffers[i] = VK_NULL_HANDLE;
		}
	}

	if (vk.renderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(vk.device, vk.renderPass, NULL);
		vk.renderPass = VK_NULL_HANDLE;
	}

	// Offscreen scene target + present pass resources
	if (vk.sceneColorDescriptor != VK_NULL_HANDLE)
	{
		vkFreeDescriptorSets(vk.device, vk.uiDescriptorPool, 1, &vk.sceneColorDescriptor);
		vk.sceneColorDescriptor = VK_NULL_HANDLE;
	}
	if (vk.gammaPipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(vk.device, vk.gammaPipeline, NULL);
		vk.gammaPipeline = VK_NULL_HANDLE;
	}
	if (vk.sceneFramebuffer != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(vk.device, vk.sceneFramebuffer, NULL);
		vk.sceneFramebuffer = VK_NULL_HANDLE;
	}
	if (vk.presentRenderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(vk.device, vk.presentRenderPass, NULL);
		vk.presentRenderPass = VK_NULL_HANDLE;
	}
	if (vk.sceneColorView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(vk.device, vk.sceneColorView, NULL);
		vk.sceneColorView = VK_NULL_HANDLE;
	}
	if (vk.sceneColorImage != VK_NULL_HANDLE)
	{
		vkDestroyImage(vk.device, vk.sceneColorImage, NULL);
		vk.sceneColorImage = VK_NULL_HANDLE;
	}
	if (vk.sceneColorMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(vk.device, vk.sceneColorMemory, NULL);
		vk.sceneColorMemory = VK_NULL_HANDLE;
	}
	if (vk.msColorView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(vk.device, vk.msColorView, NULL);
		vk.msColorView = VK_NULL_HANDLE;
	}
	if (vk.msColorImage != VK_NULL_HANDLE)
	{
		vkDestroyImage(vk.device, vk.msColorImage, NULL);
		vk.msColorImage = VK_NULL_HANDLE;
	}
	if (vk.msColorMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(vk.device, vk.msColorMemory, NULL);
		vk.msColorMemory = VK_NULL_HANDLE;
	}

	if (vk.depthImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(vk.device, vk.depthImageView, NULL);
		vk.depthImageView = VK_NULL_HANDLE;
	}
	if (vk.depthImage != VK_NULL_HANDLE)
	{
		vkDestroyImage(vk.device, vk.depthImage, NULL);
		vk.depthImage = VK_NULL_HANDLE;
	}
	if (vk.depthImageMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(vk.device, vk.depthImageMemory, NULL);
		vk.depthImageMemory = VK_NULL_HANDLE;
	}
}

static void VK_DestroySwapchainObjects(void)
{
	uint32_t i;

	if (vk.device == VK_NULL_HANDLE)
	{
		return;
	}

	VK_DestroySceneTargets();

	for (i = 0; i < vk.swapchainImageCount; i++)
	{
		if (vk.swapchainImageViews[i] != VK_NULL_HANDLE)
		{
			vkDestroyImageView(vk.device, vk.swapchainImageViews[i], NULL);
			vk.swapchainImageViews[i] = VK_NULL_HANDLE;
		}
	}

	if (vk.swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(vk.device, vk.swapchain, NULL);
		vk.swapchain = VK_NULL_HANDLE;
	}
}

qboolean VK_ResizeSwapchain(int width, int height)
{
	VK_DestroySwapchainObjects();

	if (!VK_CreateSwapchainAndViews(width, height))
	{
		return qfalse;
	}
	if (!VK_CreateRenderPassAndFramebuffers())
	{
		return qfalse;
	}

	// On first init this runs before the UI descriptor pool
	// and gamma layout exist; VK_Init creates the present resources then.
	if (vk.gammaPipelineLayout != VK_NULL_HANDLE && !VK_CreatePresentResources())
	{
		return qfalse;
	}

	return qtrue;
}

qboolean VK_SetSceneSize(int width, int height)
{
	if (!vk.initialized)
	{
		return qfalse;
	}

	if (width <= 0 || height <= 0 ||
	    ((uint32_t)width == vk.swapchainExtent.width && (uint32_t)height == vk.swapchainExtent.height))
	{
		width  = 0;
		height = 0;
	}

	if ((uint32_t)width == vk.sceneSize.width && (uint32_t)height == vk.sceneSize.height)
	{
		return qtrue;
	}

	vk.sceneSize.width  = (uint32_t)width;
	vk.sceneSize.height = (uint32_t)height;

	VK_DestroySceneTargets();

	if (!VK_CreateRenderPassAndFramebuffers() || !VK_CreatePresentResources())
	{
		return qfalse;
	}

	Ren_Print("Vulkan: render resolution %ux%u, window %ux%u\n", vk.sceneExtent.width, vk.sceneExtent.height,
	          vk.swapchainExtent.width, vk.swapchainExtent.height);

	return qtrue;
}

qboolean VK_Init(int width, int height)
{
	VkPhysicalDeviceProperties props;

	Com_Memset(&vk, 0, sizeof(vk));

	if (!ri.VKimp_GetInstanceExtensions || !ri.VKimp_CreateSurface || !ri.VKimp_GetInstanceProcAddr)
	{
		Ren_Print("Vulkan: engine does not expose VKimp_* window-system hooks\n");
		return qfalse;
	}

	if (!VK_LoadGlobalFunctions())
	{
		return qfalse;
	}

	if (!VK_CreateInstance() || !VK_LoadInstanceFunctions())
	{
		return qfalse;
	}

	if (!ri.VKimp_CreateSurface(vk.instance, (void **)&vk.surface))
	{
		Ren_Print("Vulkan: failed to create window surface\n");
		return qfalse;
	}

	if (!VK_PickPhysicalDeviceAndQueues())
	{
		return qfalse;
	}

	if (!VK_CreateLogicalDevice())
	{
		return qfalse;
	}

	if (!VK_ResizeSwapchain(width, height))
	{
		return qfalse;
	}

	if (!VK_CreateCommandObjects())
	{
		return qfalse;
	}

	// Mipmapped-image samplers follow the current texture
	// mode (gl_filter_min/max persist across vid_restart, while
	// GL_TextureMode() only reruns when r_textureMode is modified).
	vk.texFilterMin = gl_filter_min;
	vk.texFilterMag = gl_filter_max;

	// UI/2D rendering pipeline + resources.
	if (!VK_CreateUISamplers() ||
	    !VK_CreateMipSamplers() ||
	    !VK_CreateUIDescriptorPool() ||
	    !VK_CreateUIPipeline() ||
	    !VK_CreateUIArenaBuffers())
	{
		return qfalse;
	}

	// Static 3D geometry pipeline layouts + arena buffers.
	// Actual pipelines are created lazily per (stateBits, cullType) the
	// first time they're needed (see VK_GetScenePipeline/VK_GetSceneLMPipeline).
	if (!VK_CreateScenePipelineLayouts() ||
	    !VK_CreateSceneArenaBuffers())
	{
		return qfalse;
	}

	// GPU world culling pipeline (buffers come with the map), merged model batches
	VK_CreateCullPipeline();
	VK_CreateModelBatchResources();

	// Gamma/present pass
	if (!VK_CreateGammaPipelineLayout() ||
	    !VK_CreatePresentResources())
	{
		return qfalse;
	}

	vkGetPhysicalDeviceProperties(vk.physicalDevice, &props);
	Q_strncpyz(glConfig.vendor_string, "Vulkan", sizeof(glConfig.vendor_string));
	Q_strncpyz(glConfig.renderer_string, props.deviceName, sizeof(glConfig.renderer_string));
	Com_sprintf(glConfig.version_string, sizeof(glConfig.version_string), "%d.%d.%d",
	            VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
	glConfig.maxTextureSize = (int)props.limits.maxImageDimension2D;

	vk.initialized = qtrue;

	return qtrue;
}

void VK_Shutdown(void)
{
	if (vk.device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(vk.device);
		VK_FlushPendingFrees();

		if (vk.captureBuffer != VK_NULL_HANDLE)
		{
			vkUnmapMemory(vk.device, vk.captureMemory);
			vkDestroyBuffer(vk.device, vk.captureBuffer, NULL);
			vkFreeMemory(vk.device, vk.captureMemory, NULL);
			vk.captureBuffer = VK_NULL_HANDLE;
		}
	}

	if (vk.device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(vk.device);
	}

	VK_DestroySwapchainObjects();

	// UI/2D rendering resources.
	if (vk.device != VK_NULL_HANDLE)
	{
		if (vk.vertexArenaMapped)
		{
			vkUnmapMemory(vk.device, vk.vertexArenaMemory);
		}
		if (vk.indexArenaMapped)
		{
			vkUnmapMemory(vk.device, vk.indexArenaMemory);
		}
		if (vk.vertexArenaBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(vk.device, vk.vertexArenaBuffer, NULL);
		}
		if (vk.vertexArenaMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(vk.device, vk.vertexArenaMemory, NULL);
		}
		if (vk.indexArenaBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(vk.device, vk.indexArenaBuffer, NULL);
		}
		if (vk.indexArenaMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(vk.device, vk.indexArenaMemory, NULL);
		}
		{
			int pi;

			for (pi = 0; pi < vk.numUIPipelines; pi++)
			{
				vkDestroyPipeline(vk.device, vk.uiPipelines[pi].pipeline, NULL);
			}
		}
		if (vk.gammaPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.gammaPipelineLayout, NULL);
		}
		if (vk.gammaVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.gammaVertModule, NULL);
		}
		if (vk.gammaFragModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.gammaFragModule, NULL);
		}
		if (vk.uiBindlessPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.uiBindlessPipelineLayout, NULL);
		}
		if (vk.bindlessPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(vk.device, vk.bindlessPool, NULL); // frees vk.bindlessSet
		}
		if (vk.bindlessSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(vk.device, vk.bindlessSetLayout, NULL);
		}
		if (vk.uiBindlessVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.uiBindlessVertModule, NULL);
		}
		if (vk.uiBindlessFragModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.uiBindlessFragModule, NULL);
		}
		if (vk.uiPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.uiPipelineLayout, NULL);
		}
		if (vk.uiVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.uiVertModule, NULL);
		}
		if (vk.uiFragModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.uiFragModule, NULL);
		}
		if (vk.uiDescriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(vk.device, vk.uiDescriptorSetLayout, NULL);
		}
		if (vk.uiDescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(vk.device, vk.uiDescriptorPool, NULL);
		}
		if (vk.uiSamplerRepeat != VK_NULL_HANDLE)
		{
			vkDestroySampler(vk.device, vk.uiSamplerRepeat, NULL);
		}
		if (vk.mipSamplerRepeat != VK_NULL_HANDLE)
		{
			vkDestroySampler(vk.device, vk.mipSamplerRepeat, NULL);
		}
		if (vk.mipSamplerClamp != VK_NULL_HANDLE)
		{
			vkDestroySampler(vk.device, vk.mipSamplerClamp, NULL);
		}
		if (vk.uiSamplerClamp != VK_NULL_HANDLE)
		{
			vkDestroySampler(vk.device, vk.uiSamplerClamp, NULL);
		}

		// Static 3D geometry resources.
		if (vk.sceneVertexArenaMapped)
		{
			vkUnmapMemory(vk.device, vk.sceneVertexArenaMemory);
		}
		if (vk.sceneIndexArenaMapped)
		{
			vkUnmapMemory(vk.device, vk.sceneIndexArenaMemory);
		}
		if (vk.sceneVertexArenaBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(vk.device, vk.sceneVertexArenaBuffer, NULL);
		}
		if (vk.sceneVertexArenaMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(vk.device, vk.sceneVertexArenaMemory, NULL);
		}
		if (vk.sceneIndexArenaBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(vk.device, vk.sceneIndexArenaBuffer, NULL);
		}
		if (vk.sceneIndexArenaMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(vk.device, vk.sceneIndexArenaMemory, NULL);
		}
		{
			int pi;

			for (pi = 0; pi < vk.numScenePipelines; pi++)
			{
				vkDestroyPipeline(vk.device, vk.scenePipelines[pi].pipeline, NULL);
			}
			for (pi = 0; pi < vk.numSceneLMPipelines; pi++)
			{
				vkDestroyPipeline(vk.device, vk.sceneLMPipelines[pi].pipeline, NULL);
			}
		}
		if (vk.scenePipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.scenePipelineLayout, NULL);
		}
		if (vk.sceneLMPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.sceneLMPipelineLayout, NULL);
		}
		if (vk.sceneVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneVertModule, NULL);
		}
		if (vk.sceneFragModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneFragModule, NULL);
		}
		if (vk.sceneLMVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneLMVertModule, NULL);
		}
		if (vk.sceneLMFragModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneLMFragModule, NULL);
		}
		if (vk.sceneStaticVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneStaticVertModule, NULL);
		}
		if (vk.sceneLMStaticVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneLMStaticVertModule, NULL);
		}
		if (vk.sceneFoliageVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneFoliageVertModule, NULL);
		}
		if (vk.sceneFoliageFogVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneFoliageFogVertModule, NULL);
		}
		if (vk.sceneDiffuseVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneDiffuseVertModule, NULL);
		}
		if (vk.sceneSkinVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneSkinVertModule, NULL);
		}
		if (vk.sceneMD3VertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneMD3VertModule, NULL);
		}
		if (vk.meshMapped)
		{
			vkUnmapMemory(vk.device, vk.meshMemory);
		}
		if (vk.meshBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(vk.device, vk.meshBuffer, NULL);
		}
		if (vk.meshMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(vk.device, vk.meshMemory, NULL);
		}

		// GPU skinning
		if (vk.skinMapped)
		{
			vkUnmapMemory(vk.device, vk.skinMemory);
		}
		if (vk.skinBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(vk.device, vk.skinBuffer, NULL);
		}
		if (vk.skinMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(vk.device, vk.skinMemory, NULL);
		}
		if (vk.boneArenaMapped)
		{
			vkUnmapMemory(vk.device, vk.boneArenaMemory);
		}
		if (vk.boneArenaBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(vk.device, vk.boneArenaBuffer, NULL);
		}
		if (vk.boneArenaMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(vk.device, vk.boneArenaMemory, NULL);
		}
		if (vk.bonePool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(vk.device, vk.bonePool, NULL); // frees vk.boneSet
		}
		if (vk.skinPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.skinPipelineLayout, NULL);
		}
		if (vk.staticDlightPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.staticDlightPipelineLayout, NULL);
		}
		if (vk.sceneLMStaticDlightVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneLMStaticDlightVertModule, NULL);
		}
		if (vk.sceneLMDlightFragModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneLMDlightFragModule, NULL);
		}
		if (vk.scene2StageVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.scene2StageVertModule, NULL);
		}
		if (vk.scene2StageFragModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.scene2StageFragModule, NULL);
		}
		if (vk.boneSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(vk.device, vk.boneSetLayout, NULL);
		}

		VK_DestroyStaticWorldBuffer();
		VK_DestroyWorldCull();
		if (vk.cullPipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(vk.device, vk.cullPipeline, NULL);
		}
		if (vk.cullPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.cullPipelineLayout, NULL);
		}
		if (vk.cullPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(vk.device, vk.cullPool, NULL);
		}
		if (vk.cullSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(vk.device, vk.cullSetLayout, NULL);
		}
		if (vk.gatherPipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(vk.device, vk.gatherPipeline, NULL);
		}
		if (vk.sceneBindlessPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.sceneBindlessPipelineLayout, NULL);
		}
		if (vk.worldDrawPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(vk.device, vk.worldDrawPool, NULL);
		}
		if (vk.worldDrawSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(vk.device, vk.worldDrawSetLayout, NULL);
		}
		if (vk.sceneStaticBLVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneStaticBLVertModule, NULL);
		}
		if (vk.sceneLMStaticBLVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneLMStaticBLVertModule, NULL);
		}
		if (vk.sceneBLFragModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneBLFragModule, NULL);
		}
		if (vk.sceneLMBLFragModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneLMBLFragModule, NULL);
		}
		if (vk.sceneStaticFogBLVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneStaticFogBLVertModule, NULL);
		}
		if (vk.sceneMD3BLVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneMD3BLVertModule, NULL);
		}
		if (vk.sceneSkinBLVertModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(vk.device, vk.sceneSkinBLVertModule, NULL);
		}
		if (vk.sceneMD3BLPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(vk.device, vk.sceneMD3BLPipelineLayout, NULL);
		}
		if (vk.modelPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(vk.device, vk.modelPool, NULL);
		}
		if (vk.modelSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(vk.device, vk.modelSetLayout, NULL);
		}
		if (vk.modelDrawMemory != VK_NULL_HANDLE)
		{
			vkUnmapMemory(vk.device, vk.modelDrawMemory);
		}
		if (vk.modelCmdMemory != VK_NULL_HANDLE)
		{
			vkUnmapMemory(vk.device, vk.modelCmdMemory);
		}
		VK_DestroyCullBuffer(&vk.modelDrawBuffer, &vk.modelDrawMemory);
		VK_DestroyCullBuffer(&vk.modelCmdBuffer, &vk.modelCmdMemory);
	}

	{
		int i;

		for (i = 0; i < VK_FRAMES_IN_FLIGHT; i++)
		{
			if (vk.imageAvailableSemaphores[i] != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(vk.device, vk.imageAvailableSemaphores[i], NULL);
			}
			if (vk.inFlightFences[i] != VK_NULL_HANDLE)
			{
				vkDestroyFence(vk.device, vk.inFlightFences[i], NULL);
			}
		}
		for (i = 0; i < VK_MAX_SWAPCHAIN_IMAGES; i++)
		{
			if (vk.renderFinishedSemaphores[i] != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(vk.device, vk.renderFinishedSemaphores[i], NULL);
			}
		}
	}
	if (vk.commandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(vk.device, vk.commandPool, NULL);
	}
	if (vk.device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(vk.device, NULL);
	}
	if (vk.surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(vk.instance, vk.surface, NULL);
	}
	if (vk.instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(vk.instance, NULL);
	}

	Com_Memset(&vk, 0, sizeof(vk));
	VK_ClearFunctions();
}

/**
 * @brief Forgets all cached command-buffer binding state (new command
 * buffer, or after a render pass that used its own pipeline/layout).
 */
static void VK_ResetBindCache(void)
{
	vk.lastBoundPipeline = VK_NULL_HANDLE;
	vk.boundSetLayout    = VK_NULL_HANDLE;
	vk.boundSetCount     = 0;
	vk.pushLayout        = VK_NULL_HANDLE;
	vk.pushBytes         = 0;
	vk.boundVertexBuffer = VK_NULL_HANDLE;
	vk.boundIndexBuffer  = VK_NULL_HANDLE;
	vk.viewport2DSet     = qfalse;
}

static void VK_BindPipelineCached(VkPipeline pipeline)
{
	// every draw binds its pipeline first: record a pending 2D batch before it
	VK_Flush2D();

	if (vk.lastBoundPipeline != pipeline)
	{
		vkCmdBindPipeline(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vk.lastBoundPipeline = pipeline;
	}
}

/**
 * @brief Binds descriptor sets 0..count-1 unless exactly these sets are
 * already bound for this pipeline layout.
 */
static void VK_BindDescriptorSetsCached(VkPipelineLayout layout, uint32_t count, const VkDescriptorSet *sets)
{
	uint32_t i;

	if (vk.boundSetLayout == layout && vk.boundSetCount == count)
	{
		for (i = 0; i < count; i++)
		{
			if (vk.boundSets[i] != sets[i])
			{
				break;
			}
		}
		if (i == count)
		{
			return;
		}
	}

	vkCmdBindDescriptorSets(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, count, sets, 0, NULL);
	vk.boundSetLayout = layout;
	vk.boundSetCount  = count;
	for (i = 0; i < count && i < ARRAY_LEN(vk.boundSets); i++)
	{
		vk.boundSets[i] = sets[i];
	}
}

/**
 * @brief Push constants, sending only the range of floats that changed
 * since the last push with the same layout (push constant values persist
 * across draws and same-layout pipeline binds).
 */
static void VK_PushConstantsCached(VkPipelineLayout layout, VkShaderStageFlags stages, const float *data, uint32_t bytes)
{
	uint32_t n = bytes / sizeof(float), first, last;
	// floats already pushed with this layout (draws may push a prefix of
	// the range: arena draws omit the static-only color terms)
	uint32_t known = (vk.pushLayout == layout) ? vk.pushBytes / sizeof(float) : 0;

	for (first = 0; first < n && first < known && vk.pushData[first] == data[first]; first++)
	{
	}
	if (first == n)
	{
		return;
	}
	for (last = n - 1; last > first && last < known && vk.pushData[last] == data[last]; last--)
	{
	}

	vkCmdPushConstants(vk.commandBuffer, layout, stages, first * sizeof(float), (last - first + 1) * sizeof(float), data + first);
	Com_Memcpy(vk.pushData + first, data + first, (last - first + 1) * sizeof(float));
	vk.pushLayout = layout;
	vk.pushBytes  = MAX(known, n) * sizeof(float);
}

/**
 * @brief Keeps the vertex/index arena buffers bound at offset 0; draws
 * address their data with firstIndex/vertexOffset.
 */
static void VK_BindArenaBuffers(VkBuffer vertexBuffer, VkBuffer indexBuffer)
{
	if (vk.boundVertexBuffer != vertexBuffer)
	{
		VkDeviceSize zero = 0;

		vkCmdBindVertexBuffers(vk.commandBuffer, 0, 1, &vertexBuffer, &zero);
		vk.boundVertexBuffer = vertexBuffer;
	}
	if (vk.boundIndexBuffer != indexBuffer)
	{
		vkCmdBindIndexBuffer(vk.commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vk.boundIndexBuffer = indexBuffer;
	}
}

#define VK_ALIGN_UP(x, a) ((((x) + (a) - 1) / (a)) * (a))

/**
 * @brief Frees image resources queued by
 * VK_QueueImageResourcesForFree().
 */
static void VK_FlushPendingFramesSlot(int slot)
{
	int i;

	for (i = 0; i < vk.numPendingFree[slot]; i++)
	{
		if (vk.pendingFree[slot][i].descriptorSet != VK_NULL_HANDLE)
		{
			vkFreeDescriptorSets(vk.device, vk.uiDescriptorPool, 1, &vk.pendingFree[slot][i].descriptorSet);
		}
		if (vk.pendingFree[slot][i].view != VK_NULL_HANDLE)
		{
			vkDestroyImageView(vk.device, vk.pendingFree[slot][i].view, NULL);
		}
		if (vk.pendingFree[slot][i].image != VK_NULL_HANDLE)
		{
			vkDestroyImage(vk.device, vk.pendingFree[slot][i].image, NULL);
		}
		if (vk.pendingFree[slot][i].memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(vk.device, vk.pendingFree[slot][i].memory, NULL);
		}
		if (vk.pendingFree[slot][i].bindlessSlot)
		{
			vk.bindlessFree[vk.numBindlessFree++] = vk.pendingFree[slot][i].bindlessSlot - 1;
		}
	}
	vk.numPendingFree[slot] = 0;
}

/**
 * @brief Frees everything queued in all frame slots (device must be idle).
 */
static void VK_FlushPendingFrees(void)
{
	int slot;

	for (slot = 0; slot < VK_FRAMES_IN_FLIGHT; slot++)
	{
		VK_FlushPendingFramesSlot(slot);
	}
}

/**
 * @brief Detaches an image_t's Vulkan resources and frees
 * them once the frame being recorded has finished (falls back to a
 * synchronous delete if the queue is full).
 */
static void VK_QueueImageResourcesForFree(struct image_s *image)
{
	int slot = vk.frameIndex;

	if (vk.numPendingFree[slot] >= (int)ARRAY_LEN(vk.pendingFree[slot]))
	{
		VK_DeleteImageTexture(image);
		return;
	}

	vk.pendingFree[slot][vk.numPendingFree[slot]].image         = (VkImage)image->vkImage;
	vk.pendingFree[slot][vk.numPendingFree[slot]].memory        = (VkDeviceMemory)image->vkImageMemory;
	vk.pendingFree[slot][vk.numPendingFree[slot]].view          = (VkImageView)image->vkImageView;
	vk.pendingFree[slot][vk.numPendingFree[slot]].descriptorSet = (VkDescriptorSet)image->vkDescriptorSet;
	vk.pendingFree[slot][vk.numPendingFree[slot]].bindlessSlot  = image->vkBindlessSlot;
	vk.numPendingFree[slot]++;
	image->vkBindlessSlot = 0;

	image->vkImage         = NULL;
	image->vkImageMemory   = NULL;
	image->vkImageView     = NULL;
	image->vkDescriptorSet = NULL;
	image->vkSampler       = NULL;
}

/**
 * @brief Clears a freshly created persistent color target to black and
 * transitions it to finalLayout, recorded into the frame command buffer
 * before the main render pass (which loads it).
 */
static void VK_InitSceneColorImage(VkImage image, VkImageLayout finalLayout, VkAccessFlags dstAccess, VkPipelineStageFlags dstStage)
{
	VkImageMemoryBarrier    barrier;
	VkClearColorValue       black;
	VkImageSubresourceRange range;

	Com_Memset(&range, 0, sizeof(range));
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.levelCount = 1;
	range.layerCount = 1;

	Com_Memset(&barrier, 0, sizeof(barrier));
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = image;
	barrier.subresourceRange    = range;
	barrier.srcAccessMask       = 0;
	barrier.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
	vkCmdPipelineBarrier(vk.commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	Com_Memset(&black, 0, sizeof(black));
	black.float32[3] = 1.0f;
	vkCmdClearColorImage(vk.commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &black, 1, &range);

	barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout     = finalLayout;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = dstAccess;
	vkCmdPipelineBarrier(vk.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, dstStage,
	                     0, 0, NULL, 0, NULL, 1, &barrier);
}

void VK_BeginFrame(void)
{
	VkCommandBufferBeginInfo beginInfo;
	VkRenderPassBeginInfo    renderPassInfo;
	VkClearValue             clearValues[2];

	if (!vk.initialized)
	{
		return;
	}

	// advance to the next frame slot and wait until the GPU is done with the
	// frame that last used it (VK_FRAMES_IN_FLIGHT frames ago)
	vk.frameIndex              = (vk.frameIndex + 1) % VK_FRAMES_IN_FLIGHT;
	vk.commandBuffer           = vk.commandBuffers[vk.frameIndex];
	vk.inFlightFence           = vk.inFlightFences[vk.frameIndex];
	vk.imageAvailableSemaphore = vk.imageAvailableSemaphores[vk.frameIndex];

	// Reset the per-frame vertex/index arenas (this slot's region).
	vk.vertexArenaOffset = (VkDeviceSize)VK_UI_VERTEX_ARENA_SIZE * vk.frameIndex;
	vk.indexArenaOffset  = (VkDeviceSize)VK_UI_INDEX_ARENA_SIZE * vk.frameIndex;
	vk.vertexArenaSize   = vk.vertexArenaOffset + VK_UI_VERTEX_ARENA_SIZE; // end of region
	vk.indexArenaSize    = vk.indexArenaOffset + VK_UI_INDEX_ARENA_SIZE;

	// Same for the 3D scene arenas. lastBoundPipeline is shared
	// across UI and scene draws (viewport/scissor are set per-view, not
	// cached per-frame - see VK_SetSceneViewport(), called from
	// tr_backend.c's SetViewportAndScissor()).
	vk.sceneVertexArenaOffset = (VkDeviceSize)VK_SCENE_VERTEX_ARENA_SIZE * vk.frameIndex;
	vk.sceneIndexArenaOffset  = (VkDeviceSize)VK_SCENE_INDEX_ARENA_SIZE * vk.frameIndex;
	vk.sceneVertexArenaSize   = vk.sceneVertexArenaOffset + VK_SCENE_VERTEX_ARENA_SIZE; // end of region
	vk.sceneIndexArenaSize    = vk.sceneIndexArenaOffset + VK_SCENE_INDEX_ARENA_SIZE;
	vk.batch2DOpen            = qfalse; // VK_EndFrame recorded the last one
	vk.frameCount++;
	vk.cullRecording   = qfalse;
	vk.cullNumViews    = 0;
	vk.wbFrameDraws    = 0;
	vk.wbNumKeys       = 0;
	vk.wbNumDraws      = 0;
	vk.mbFrameDraws    = 0;
	vk.mbNumKeys       = 0;
	vk.mbNumDraws      = 0;
	vk.boneArenaOffset = (VkDeviceSize)VK_SKIN_BONE_ARENA_SIZE * vk.frameIndex;
	vk.boneArenaSize   = vk.boneArenaOffset + VK_SKIN_BONE_ARENA_SIZE;
	VK_ResetBindCache();

	vkWaitForFences(vk.device, 1, &vk.inFlightFence, VK_TRUE, UINT64_MAX);

	// That frame is done, so resources queued while recording
	// it can go now
	VK_FlushPendingFramesSlot(vk.frameIndex);
	vkResetFences(vk.device, 1, &vk.inFlightFence);

	vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, vk.imageAvailableSemaphore, VK_NULL_HANDLE, &vk.currentImageIndex);
	vk.renderFinishedSemaphore = vk.renderFinishedSemaphores[vk.currentImageIndex];

	vkResetCommandBuffer(vk.commandBuffer, 0);

	Com_Memset(&beginInfo, 0, sizeof(beginInfo));
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(vk.commandBuffer, &beginInfo);

	// the persistent color target(s) start out undefined: clear to black
	// once and move them into the layout the main render pass loads from
	if (vk.sceneColorNeedsInit)
	{
		VK_InitSceneColorImage(vk.sceneColorImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		                       VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		if (vk.msColorImage != VK_NULL_HANDLE)
		{
			VK_InitSceneColorImage(vk.msColorImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			                       VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		}
		vk.sceneColorNeedsInit = qfalse;
	}

	Com_Memset(clearValues, 0, sizeof(clearValues));
	clearValues[0].color.float32[0]     = 0.0f;
	clearValues[0].color.float32[1]     = 0.0f;
	clearValues[0].color.float32[2]     = 0.0f;
	clearValues[0].color.float32[3]     = 1.0f;
	clearValues[1].depthStencil.depth   = 1.0f;
	clearValues[1].depthStencil.stencil = 0;

	Com_Memset(&renderPassInfo, 0, sizeof(renderPassInfo));
	renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass        = vk.renderPass;
	renderPassInfo.framebuffer       = vk.sceneFramebuffer; // Offscreen scene target
	renderPassInfo.renderArea.extent = vk.sceneExtent;
	renderPassInfo.clearValueCount   = 2;
	renderPassInfo.pClearValues      = clearValues;

	vkCmdBeginRenderPass(vk.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vk.inFrame = qtrue;
}

/**
 * @brief Debug capture: writes the frame VK_EndFrame() just copied into the
 * capture buffer to a PPM file for offline visual inspection (our own
 * rendered image, not the user's display).
 */
static void VK_DebugWriteCapturePPM(const char *path)
{
	const byte *src = (const byte *)vk.captureMapped;
	qboolean   bgra = (vk.captureFormat == VK_FORMAT_B8G8R8A8_UNORM || vk.captureFormat == VK_FORMAT_B8G8R8A8_SRGB);
	byte       *row;
	FILE       *f;
	uint32_t   x, y;

	if (!src)
	{
		return;
	}

	row = malloc((size_t)vk.captureWidth * 3);
	f   = fopen(path, "wb");
	if (!row || !f)
	{
		free(row);
		if (f)
		{
			fclose(f);
		}
		Ren_Print("VK_DebugWriteCapturePPM: could not write %s\n", path);
		return;
	}

	fprintf(f, "P6\n%u %u\n255\n", vk.captureWidth, vk.captureHeight);
	for (y = 0; y < vk.captureHeight; y++)
	{
		const byte *p = src + (size_t)y * vk.captureWidth * 4;

		for (x = 0; x < vk.captureWidth; x++, p += 4)
		{
			row[x * 3 + 0] = bgra ? p[2] : p[0];
			row[x * 3 + 1] = p[1];
			row[x * 3 + 2] = bgra ? p[0] : p[2];
		}
		fwrite(row, 3, vk.captureWidth, f);
	}
	fclose(f);
	free(row);

	Ren_Print("VK_DebugWriteCapturePPM: wrote %s\n", path);
}

/**
 * @brief (re)creates the persistently mapped capture buffer
 * for the current swapchain size.
 */
static qboolean VK_EnsureCaptureBuffer(void)
{
	VkDeviceSize size = (VkDeviceSize)vk.swapchainExtent.width * vk.swapchainExtent.height * 4;

	if (vk.captureBuffer != VK_NULL_HANDLE && vk.captureSize == size)
	{
		return qtrue;
	}

	if (vk.captureBuffer != VK_NULL_HANDLE)
	{
		vkUnmapMemory(vk.device, vk.captureMemory);
		vkDestroyBuffer(vk.device, vk.captureBuffer, NULL);
		vkFreeMemory(vk.device, vk.captureMemory, NULL);
		vk.captureBuffer = VK_NULL_HANDLE;
		vk.captureMemory = VK_NULL_HANDLE;
		vk.captureMapped = NULL;
	}

	if (!VK_CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &vk.captureBuffer, &vk.captureMemory))
	{
		vk.captureBuffer = VK_NULL_HANDLE;
		return qfalse;
	}

	vkMapMemory(vk.device, vk.captureMemory, 0, size, 0, &vk.captureMapped);
	vk.captureSize = size;

	return qtrue;
}

/**
 * @brief Asks VK_EndFrame() to capture the frame currently
 * being recorded (or the next one, if called between frames).
 */
void VK_RequestFrameCapture(void)
{
	vk.captureRequested = qtrue;
}

/**
 * @brief Size of the last captured frame, or qfalse if none
 * is pending.
 */
qboolean VK_GetCapturedFrameSize(int *width, int *height)
{
	if (!vk.captureReady || !vk.captureMapped)
	{
		return qfalse;
	}

	*width  = (int)vk.captureWidth;
	*height = (int)vk.captureHeight;
	return qtrue;
}

/**
 * @brief Copies the captured frame as glReadPixels(GL_RGB)
 * would return it: rows bottom-up, 3 bytes per pixel, each row followed
 * by padlen bytes. width/height may be smaller than the capture (crop).
 */
void VK_ReadCapturedFrameRGB(byte *dst, int width, int height, int padlen)
{
	const byte *src = (const byte *)vk.captureMapped;
	qboolean   bgra = (vk.captureFormat == VK_FORMAT_B8G8R8A8_UNORM || vk.captureFormat == VK_FORMAT_B8G8R8A8_SRGB);
	int        x, y;

	if (!vk.captureReady || !src)
	{
		return;
	}

	if (width > (int)vk.captureWidth)
	{
		width = (int)vk.captureWidth;
	}
	if (height > (int)vk.captureHeight)
	{
		height = (int)vk.captureHeight;
	}

	for (y = 0; y < height; y++)
	{
		// GL row 0 is the bottom of the image, Vulkan row 0 the top
		const byte *row = src + (size_t)(vk.captureHeight - 1 - y) * vk.captureWidth * 4;

		for (x = 0; x < width; x++, row += 4)
		{
			*dst++ = bgra ? row[2] : row[0];
			*dst++ = row[1];
			*dst++ = bgra ? row[0] : row[2];
		}
		dst += padlen;
	}
}

/**
 * @brief Marks the captured frame as consumed.
 */
void VK_ReleaseCapturedFrame(void)
{
	vk.captureReady = qfalse;
}

void VK_EndFrame(void)
{
	VkCommandBuffer      commandBuffers[2];
	VkSubmitInfo         submitInfo;
	VkPresentInfoKHR     presentInfo;
	VkPipelineStageFlags waitStage         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	static int           debugFrameCount   = 0;
	const char           *debugCapturePath = NULL;
	qboolean             captureThisFrame;

	if (!vk.initialized)
	{
		return;
	}

	VK_Flush2D(); // keep the pending 2D batch before this in the command buffer

	vkCmdEndRenderPass(vk.commandBuffer);

	// renderer1's R_ScreenGamma() - copy the offscreen scene
	// to the swapchain image applying r_gamma and the overbright scale.
	{
		VkRenderPassBeginInfo presentInfoRP;
		VkViewport            viewport;
		VkRect2D              scissor;
		float                 push[2];

		Com_Memset(&presentInfoRP, 0, sizeof(presentInfoRP));
		presentInfoRP.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		presentInfoRP.renderPass        = vk.presentRenderPass;
		presentInfoRP.framebuffer       = vk.swapchainFramebuffers[vk.currentImageIndex];
		presentInfoRP.renderArea.extent = vk.swapchainExtent;

		vkCmdBeginRenderPass(vk.commandBuffer, &presentInfoRP, VK_SUBPASS_CONTENTS_INLINE);

		Com_Memset(&viewport, 0, sizeof(viewport));
		viewport.width    = (float)vk.swapchainExtent.width;
		viewport.height   = (float)vk.swapchainExtent.height;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(vk.commandBuffer, 0, 1, &viewport);

		Com_Memset(&scissor, 0, sizeof(scissor));
		scissor.extent = vk.swapchainExtent;
		vkCmdSetScissor(vk.commandBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.gammaPipeline);
		vk.lastBoundPipeline = vk.gammaPipeline;
		vkCmdBindDescriptorSets(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.gammaPipelineLayout, 0, 1, &vk.sceneColorDescriptor, 0, NULL);

		push[0] = vk.gammaValue;
		push[1] = vk.overbrightScale;
		vkCmdPushConstants(vk.commandBuffer, vk.gammaPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), push);

		vkCmdDraw(vk.commandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(vk.commandBuffer);
	}

	// debug capture of frame 200 (r_vkCaptureFrame200). It shares the
	// screenshot capture below and is skipped while a captured screenshot
	// frame is still unread.
	debugFrameCount++;
	if (debugFrameCount == 200 && r_vkCaptureFrame200->integer && !vk.captureReady)
	{
		debugCapturePath = r_vkCapturePath->string;
	}

	// Frame capture (renderer1's glReadPixels of the final
	// image) - copy the swapchain image into the host-visible capture
	// buffer before it's presented.
	captureThisFrame = (vk.captureRequested || debugCapturePath) && VK_EnsureCaptureBuffer();
	if (captureThisFrame)
	{
		VkImageMemoryBarrier barrier;
		VkBufferImageCopy    region;

		Com_Memset(&barrier, 0, sizeof(barrier));
		barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout                   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
		barrier.image                       = vk.swapchainImages[vk.currentImageIndex];
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask               = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask               = VK_ACCESS_TRANSFER_READ_BIT;
		vkCmdPipelineBarrier(vk.commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		                     0, 0, NULL, 0, NULL, 1, &barrier);

		Com_Memset(&region, 0, sizeof(region));
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.layerCount = 1;
		region.imageExtent.width           = vk.swapchainExtent.width;
		region.imageExtent.height          = vk.swapchainExtent.height;
		region.imageExtent.depth           = 1;
		vkCmdCopyImageToBuffer(vk.commandBuffer, vk.swapchainImages[vk.currentImageIndex], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		                       vk.captureBuffer, 1, &region);

		barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = 0;
		vkCmdPipelineBarrier(vk.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		                     0, 0, NULL, 0, NULL, 1, &barrier);
	}

	vkEndCommandBuffer(vk.commandBuffer);

	// GPU world culling: the frame's dispatches run first; the barrier makes
	// their draw commands and indexes visible to the main command buffer's
	// draws (later in submission order)
	commandBuffers[0] = vk.commandBuffer;
	if (vk.cullRecording)
	{
		VkMemoryBarrier barrier;

		// merged world batches: gather their group commands, after the
		// culling dispatches wrote them
		if (vk.wbFrameDraws)
		{
			VkCommandBuffer cb = vk.cullCommandBuffers[vk.frameIndex];
			unsigned int    push[2];

			Com_Memset(&barrier, 0, sizeof(barrier));
			barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, NULL, 0, NULL);

			push[0] = (unsigned int)(vk.frameIndex * vk.cullMaxBatchDraws);
			push[1] = (unsigned int)vk.wbFrameDraws;
			vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, vk.gatherPipeline);
			vkCmdPushConstants(cb, vk.cullPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), push);
			vkCmdDispatch(cb, (uint32_t)(vk.wbFrameDraws + 63) / 64, 1, 1);
		}

		Com_Memset(&barrier, 0, sizeof(barrier));
		barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT;
		vkCmdPipelineBarrier(vk.cullCommandBuffers[vk.frameIndex], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		                     VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 1, &barrier, 0, NULL, 0, NULL);
		vkEndCommandBuffer(vk.cullCommandBuffers[vk.frameIndex]);
		commandBuffers[0] = vk.cullCommandBuffers[vk.frameIndex];
		commandBuffers[1] = vk.commandBuffer;
		vk.cullRecording  = qfalse;
	}
	vk.inFrame = qfalse;

	Com_Memset(&submitInfo, 0, sizeof(submitInfo));
	submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount   = 1;
	submitInfo.pWaitSemaphores      = &vk.imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask    = &waitStage;
	submitInfo.commandBufferCount   = (commandBuffers[0] == vk.commandBuffer) ? 1 : 2;
	submitInfo.pCommandBuffers      = commandBuffers;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores    = &vk.renderFinishedSemaphore;

	vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, vk.inFlightFence);

	Com_Memset(&presentInfo, 0, sizeof(presentInfo));
	presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores    = &vk.renderFinishedSemaphore;
	presentInfo.swapchainCount     = 1;
	presentInfo.pSwapchains        = &vk.swapchain;
	presentInfo.pImageIndices      = &vk.currentImageIndex;

	vkQueuePresentKHR(vk.presentQueue, &presentInfo);

	if (captureThisFrame)
	{
		// the fence stays signaled, so VK_BeginFrame()'s wait returns at once
		vkWaitForFences(vk.device, 1, &vk.inFlightFence, VK_TRUE, UINT64_MAX);
		vk.captureWidth  = vk.swapchainExtent.width;
		vk.captureHeight = vk.swapchainExtent.height;
		vk.captureFormat = vk.swapchainFormat;

		if (debugCapturePath)
		{
			VK_DebugWriteCapturePPM(debugCapturePath);
		}

		// a debug-only capture must not hand a frame to a later screenshot
		if (vk.captureRequested)
		{
			vk.captureReady     = qtrue;
			vk.captureRequested = qfalse;
		}
	}
}

/**
 * @brief Uploads a 32-bit RGBA image to a new device-local
 * VkImage via a host-visible staging buffer and a synchronous one-shot
 * transfer command buffer, then creates its view and descriptor set
 * (sampler picked by mipmap/wrapClampMode). Stores the new handles on the
 * image_t itself.
 *
 * @note Always uploads as VK_FORMAT_R8G8B8A8_UNORM regardless of the
 * legacy GL internalFormat selection. Uploads the mipLevels packed levels
 * in data (built CPU-side by Upload32()).
 */
qboolean VK_CreateImageTexture(struct image_s *image, const byte *data, int width, int height, int mipLevels, qboolean mipmap, int wrapClampMode)
{
	VkDeviceSize                imageSize     = 0;
	VkBuffer                    stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory              stagingMemory = VK_NULL_HANDLE;
	void                        *mapped;
	VkImageCreateInfo           imageInfo;
	VkMemoryRequirements        memRequirements;
	VkMemoryAllocateInfo        allocInfo;
	VkImage                     vkImage       = VK_NULL_HANDLE;
	VkDeviceMemory              vkImageMemory = VK_NULL_HANDLE;
	VkCommandBuffer             cmd;
	VkImageMemoryBarrier        barrier;
	VkImageViewCreateInfo       viewInfo;
	VkImageView                 vkImageView = VK_NULL_HANDLE;
	VkDescriptorSetAllocateInfo dsAllocInfo;
	VkDescriptorSet             descriptorSet = VK_NULL_HANDLE;
	VkDescriptorImageInfo       imageDescInfo;
	VkWriteDescriptorSet        writeSet;
	VkSampler                   sampler;
	VkBufferImageCopy           regions[16];
	int                         level, levelW, levelH;

	if (!vk.initialized || !data || width <= 0 || height <= 0)
	{
		return qfalse;
	}

	// Data holds mipLevels tightly packed RGBA levels, each
	// half the size of the previous one (clamped to 1), as built by
	// Upload32() with renderer1's R_MipMap().
	if (mipLevels < 1)
	{
		mipLevels = 1;
	}
	if (mipLevels > (int)ARRAY_LEN(regions))
	{
		mipLevels = (int)ARRAY_LEN(regions);
	}

	Com_Memset(regions, 0, sizeof(regions));
	levelW = width;
	levelH = height;
	for (level = 0; level < mipLevels; level++)
	{
		regions[level].bufferOffset                = imageSize;
		regions[level].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		regions[level].imageSubresource.mipLevel   = (uint32_t)level;
		regions[level].imageSubresource.layerCount = 1;
		regions[level].imageExtent.width           = (uint32_t)levelW;
		regions[level].imageExtent.height          = (uint32_t)levelH;
		regions[level].imageExtent.depth           = 1;

		imageSize += (VkDeviceSize)levelW * (VkDeviceSize)levelH * 4;

		levelW = (levelW > 1) ? levelW >> 1 : 1;
		levelH = (levelH > 1) ? levelH >> 1 : 1;
	}

	if (!VK_CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &stagingBuffer, &stagingMemory))
	{
		return qfalse;
	}

	vkMapMemory(vk.device, stagingMemory, 0, imageSize, 0, &mapped);
	Com_Memcpy(mapped, data, (size_t)imageSize);
	vkUnmapMemory(vk.device, stagingMemory);

	Com_Memset(&imageInfo, 0, sizeof(imageInfo));
	imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width  = (uint32_t)width;
	imageInfo.extent.height = (uint32_t)height;
	imageInfo.extent.depth  = 1;
	imageInfo.mipLevels     = (uint32_t)mipLevels;
	imageInfo.arrayLayers   = 1;
	imageInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

	if (!VK_CheckResult(vkCreateImage(vk.device, &imageInfo, NULL, &vkImage), "vkCreateImage"))
	{
		vkDestroyBuffer(vk.device, stagingBuffer, NULL);
		vkFreeMemory(vk.device, stagingMemory, NULL);
		return qfalse;
	}

	vkGetImageMemoryRequirements(vk.device, vkImage, &memRequirements);

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = VK_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (!VK_CheckResult(vkAllocateMemory(vk.device, &allocInfo, NULL, &vkImageMemory), "vkAllocateMemory(image)"))
	{
		vkDestroyImage(vk.device, vkImage, NULL);
		vkDestroyBuffer(vk.device, stagingBuffer, NULL);
		vkFreeMemory(vk.device, stagingMemory, NULL);
		return qfalse;
	}

	vkBindImageMemory(vk.device, vkImage, vkImageMemory, 0);

	cmd = VK_BeginOneShotCommands();

	Com_Memset(&barrier, 0, sizeof(barrier));
	barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
	barrier.image                       = vkImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = (uint32_t)mipLevels;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask               = 0;
	barrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	vkCmdCopyBufferToImage(cmd, stagingBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)mipLevels, regions);

	barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	VK_EndOneShotCommands(cmd);

	vkDestroyBuffer(vk.device, stagingBuffer, NULL);
	vkFreeMemory(vk.device, stagingMemory, NULL);

	Com_Memset(&viewInfo, 0, sizeof(viewInfo));
	viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image                       = vkImage;
	viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format                      = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.levelCount = (uint32_t)mipLevels;
	viewInfo.subresourceRange.layerCount = 1;

	if (!VK_CheckResult(vkCreateImageView(vk.device, &viewInfo, NULL, &vkImageView), "vkCreateImageView"))
	{
		vkDestroyImage(vk.device, vkImage, NULL);
		vkFreeMemory(vk.device, vkImageMemory, NULL);
		return qfalse;
	}

	sampler = VK_SamplerForImage(mipmap, wrapClampMode);

	Com_Memset(&dsAllocInfo, 0, sizeof(dsAllocInfo));
	dsAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	dsAllocInfo.descriptorPool     = vk.uiDescriptorPool;
	dsAllocInfo.descriptorSetCount = 1;
	dsAllocInfo.pSetLayouts        = &vk.uiDescriptorSetLayout;

	if (!VK_CheckResult(vkAllocateDescriptorSets(vk.device, &dsAllocInfo, &descriptorSet), "vkAllocateDescriptorSets"))
	{
		vkDestroyImageView(vk.device, vkImageView, NULL);
		vkDestroyImage(vk.device, vkImage, NULL);
		vkFreeMemory(vk.device, vkImageMemory, NULL);
		return qfalse;
	}

	Com_Memset(&imageDescInfo, 0, sizeof(imageDescInfo));
	imageDescInfo.sampler     = sampler;
	imageDescInfo.imageView   = vkImageView;
	imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	Com_Memset(&writeSet, 0, sizeof(writeSet));
	writeSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeSet.dstSet          = descriptorSet;
	writeSet.dstBinding      = 0;
	writeSet.descriptorCount = 1;
	writeSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeSet.pImageInfo      = &imageDescInfo;

	vkUpdateDescriptorSets(vk.device, 1, &writeSet, 0, NULL);

	image->vkImage         = (void *)vkImage;
	image->vkImageMemory   = (void *)vkImageMemory;
	image->vkImageView     = (void *)vkImageView;
	image->vkSampler       = (void *)sampler; // not owned here: one of the shared samplers
	image->vkDescriptorSet = (void *)descriptorSet;

	// bindless slot for the 2D path (a new slot: a replaced image's old one
	// may still be in use by frames in flight, see VK_QueueImageResourcesForFree)
	if (vk.bindless && !image->vkBindlessSlot)
	{
		if (vk.numBindlessFree > 0)
		{
			image->vkBindlessSlot = vk.bindlessFree[--vk.numBindlessFree] + 1;
			VK_BindlessWrite(image->vkBindlessSlot - 1, vkImageView, sampler);
		}
		else
		{
			Ren_Print("Vulkan: bindless texture array full, %s won't be drawn in 2D\n", image->imgName);
		}
	}

	return qtrue;
}

/**
 * @brief Tears down the Vulkan resources owned by an image_t (created by
 * VK_CreateImageTexture); safe to call on an image that was never uploaded.
 * Synchronous (vkDeviceWaitIdle) since texture purges happen outside the
 * per-frame hot path.
 */
void VK_DeleteImageTexture(struct image_s *image)
{
	if (!image || !vk.initialized || vk.device == VK_NULL_HANDLE)
	{
		return;
	}

	if (!image->vkImage && !image->vkImageView && !image->vkDescriptorSet && !image->vkImageMemory)
	{
		return;
	}

	vkDeviceWaitIdle(vk.device);

	if (image->vkDescriptorSet)
	{
		VkDescriptorSet ds = (VkDescriptorSet)image->vkDescriptorSet;
		vkFreeDescriptorSets(vk.device, vk.uiDescriptorPool, 1, &ds);
		image->vkDescriptorSet = NULL;
	}
	if (image->vkImageView)
	{
		vkDestroyImageView(vk.device, (VkImageView)image->vkImageView, NULL);
		image->vkImageView = NULL;
	}
	if (image->vkImage)
	{
		vkDestroyImage(vk.device, (VkImage)image->vkImage, NULL);
		image->vkImage = NULL;
	}
	if (image->vkImageMemory)
	{
		vkFreeMemory(vk.device, (VkDeviceMemory)image->vkImageMemory, NULL);
		image->vkImageMemory = NULL;
	}
	image->vkSampler = NULL; // shared sampler, not owned by this image

	if (image->vkBindlessSlot)
	{
		vk.bindlessFree[vk.numBindlessFree++] = image->vkBindlessSlot - 1; // the device is idle
		image->vkBindlessSlot                 = 0;
	}
}

/**
 * @brief Converts the engine's GLS_ATEST_* state bits into the alphaTest.x
 * mode value consumed by tr3d.frag.glsl / ui.frag.glsl (0=off, 1=GT_0,
 * 2=LT_80, 3=GE_80).
 */
static float VK_AlphaTestMode(unsigned long stateBits)
{
	switch (stateBits & GLS_ATEST_BITS)
	{
	case GLS_ATEST_GT_0: return 1.0f;
	case GLS_ATEST_LT_80: return 2.0f;
	case GLS_ATEST_GE_80: return 3.0f;
	default: return 0.0f;
	}
}

/**
 * @brief 2D state for a UI draw: full-window viewport/scissor (dynamic state
 * that 3D sub-views change; vk.viewport2DSet tracks it) and the push
 * constants (pixel -> clip transform, alpha test from stateBits).
 */
static void VK_Set2DState(VkPipelineLayout layout, unsigned long stateBits)
{
	float pushConstants[8];

	// Always set the full-window viewport/scissor for 2D/UI draws: viewport/
	// scissor are dynamic command-buffer state that persists across pipeline
	// binds, so if a 3D sub-view (e.g. the limbo menu's small player-model
	// preview) sets a smaller viewport earlier in this same frame, caching
	// "set once per frame" here would leave later UI draws incorrectly
	// clipped to that stale, smaller rect.
	if (!vk.viewport2DSet)
	{
		VkViewport viewport;
		VkRect2D   scissor;

		Com_Memset(&viewport, 0, sizeof(viewport));
		viewport.width    = (float)glConfig.vidWidth;
		viewport.height   = (float)glConfig.vidHeight;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(vk.commandBuffer, 0, 1, &viewport);

		Com_Memset(&scissor, 0, sizeof(scissor));
		scissor.extent.width  = (uint32_t)glConfig.vidWidth;
		scissor.extent.height = (uint32_t)glConfig.vidHeight;
		vkCmdSetScissor(vk.commandBuffer, 0, 1, &scissor);

		vk.viewport2DSet = qtrue;
	}

	pushConstants[0] = 2.0f / (float)glConfig.vidWidth;
	pushConstants[1] = 2.0f / (float)glConfig.vidHeight;
	pushConstants[2] = -1.0f;
	pushConstants[3] = -1.0f;
	pushConstants[4] = VK_AlphaTestMode(stateBits);
	pushConstants[5] = 0.0f;
	pushConstants[6] = 0.0f;
	pushConstants[7] = 0.0f;
	VK_PushConstantsCached(layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants, sizeof(pushConstants));
}

/**
 * @brief Records the pending 2D batch (vk.bindless) as one draw: consecutive
 * VK_Draw2D() calls with the same stateBits, any textures (each vertex
 * carries its bindless slot). Called before anything else is recorded into
 * the frame's command buffer, so the draw order stays renderer1's; within a
 * draw, primitives blend in order.
 */
static void VK_Flush2DBatch(void)
{
	VkPipeline pipeline;

	if (!vk.batch2DOpen)
	{
		return;
	}
	vk.batch2DOpen = qfalse;

	pipeline = VK_GetUIPipeline(vk.batch2DStateBits);
	if (pipeline == VK_NULL_HANDLE || !vk.batch2DNumIndexes)
	{
		return;
	}

	VK_BindPipelineCached(pipeline);
	VK_Set2DState(vk.uiBindlessPipelineLayout, vk.batch2DStateBits);
	VK_BindDescriptorSetsCached(vk.uiBindlessPipelineLayout, 1, &vk.bindlessSet);
	VK_BindArenaBuffers(vk.vertexArenaBuffer, vk.indexArenaBuffer);
	vkCmdDrawIndexed(vk.commandBuffer, vk.batch2DNumIndexes, 1, vk.batch2DFirstIndex, vk.batch2DVertexOffset, 0);
}

/**
 * @brief Records what is pending: the merged world or model batch (only
 * one of them is pending: each flushes the other when it starts, see
 * VK_BatchWorldDraw/VK_BatchModelDraw), then the 2D batch (newer). Called before anything else is
 * recorded into the frame's command buffer.
 */
static void VK_Flush2D(void)
{
	VK_FlushWorldBatch();
	VK_FlushModelBatch();
	VK_Flush2DBatch();
}

/**
 * @brief VK_Draw2D() with bindless textures: appends the batch's vertices
 * (with the image's slot) and indexes to the pending 2D batch, starting a
 * new one when the state differs (VK_Flush2D()).
 */
static void VK_Draw2DBatched(shaderCommands_t *input, struct image_s *image, unsigned long stateBits)
{
	int          numVertexes = input->numVertexes, numIndexes = input->numIndexes, i;
	VkDeviceSize vertexBytes = (VkDeviceSize)numVertexes * sizeof(vkUIVertex_t);
	VkDeviceSize indexBytes  = (VkDeviceSize)numIndexes * sizeof(uint32_t);
	vkUIVertex_t *dstVerts;
	uint32_t     *dstIndexes;
	uint32_t     texture, base;

	if (!image->vkBindlessSlot)
	{
		return; // the bindless array was full when it was created
	}
	texture = (uint32_t)(image->vkBindlessSlot - 1);

	if (vk.batch2DOpen && stateBits != vk.batch2DStateBits)
	{
		VK_Flush2D();
	}

	if (!vk.batch2DOpen)
	{
		// draws address the arena with vertexOffset (in vertices): keep the
		// byte offset a multiple of this vertex type's stride
		vk.vertexArenaOffset = VK_ALIGN_UP(vk.vertexArenaOffset, sizeof(vkUIVertex_t));
	}

	if (vk.vertexArenaOffset + vertexBytes > vk.vertexArenaSize ||
	    vk.indexArenaOffset + indexBytes > vk.indexArenaSize)
	{
		// rate-limited, see VK_Draw2D
		static int lastWarnFrame = -1;

		if (backEnd.refdef.time != lastWarnFrame)
		{
			Ren_Print("Vulkan: VK_Draw2D - UI vertex/index arena exhausted this frame, dropping remaining 2D draws\n");
			lastWarnFrame = backEnd.refdef.time;
		}
		return;
	}

	if (!vk.batch2DOpen)
	{
		VK_FlushWorldBatch(); // pending world and model batches go first
		VK_FlushModelBatch();
		vk.batch2DOpen         = qtrue;
		vk.batch2DStateBits    = stateBits;
		vk.batch2DFirstIndex   = (uint32_t)(vk.indexArenaOffset / sizeof(uint32_t));
		vk.batch2DVertexOffset = (int32_t)(vk.vertexArenaOffset / sizeof(vkUIVertex_t));
		vk.batch2DNumIndexes   = 0;
		vk.batch2DNumVertexes  = 0;
	}

	dstVerts = (vkUIVertex_t *)((byte *)vk.vertexArenaMapped + vk.vertexArenaOffset);
	for (i = 0; i < numVertexes; i++)
	{
		dstVerts[i].pos[0]   = input->xyz[i][0];
		dstVerts[i].pos[1]   = input->xyz[i][1];
		dstVerts[i].uv[0]    = input->svars.texcoords[0][i][0];
		dstVerts[i].uv[1]    = input->svars.texcoords[0][i][1];
		dstVerts[i].color[0] = input->svars.colors[i][0];
		dstVerts[i].color[1] = input->svars.colors[i][1];
		dstVerts[i].color[2] = input->svars.colors[i][2];
		dstVerts[i].color[3] = input->svars.colors[i][3];
		dstVerts[i].texture  = texture;
	}

	base       = (uint32_t)vk.batch2DNumVertexes;
	dstIndexes = (uint32_t *)((byte *)vk.indexArenaMapped + vk.indexArenaOffset);
	for (i = 0; i < numIndexes; i++)
	{
		dstIndexes[i] = input->indexes[i] + base;
	}

	vk.batch2DNumVertexes += numVertexes;
	vk.batch2DNumIndexes  += (uint32_t)numIndexes;
	vk.vertexArenaOffset  += vertexBytes;
	vk.indexArenaOffset   += indexBytes;
}

/**
 * @brief Appends one shader batch's worth of already-tessellated 2D/UI
 * geometry (tess.xyz/texCoords[0]/vertexColors/indexes) into this frame's
 * vertex/index arenas and issues one vkCmdDrawIndexed with the UI pipeline
 * variant matching `stateBits` bound to the given image's descriptor set.
 * Honoring the real stateBits (rather than always forcing a fixed
 * blend-only state) is required for UI tricks that rely on depth test/
 * func/write or alpha test, e.g. the automap's circular-mask trick in
 * cg_commandmap.c (alphafunc GE128 + depthFunc equal against the 3D
 * scene's still-resident depth buffer, since 2D draws share the same
 * render pass/depth attachment as the 3D scene).
 *
 * @note Draws a single texture bundle; the caller (RB_IterateStages2D())
 * picks the animated image frame and issues one call per bundle.
 */
void VK_Draw2D(shaderCommands_t *input, struct image_s *image, unsigned long stateBits)
{
	int             numVertexes, numIndexes, i;
	VkDeviceSize    vertexBytes, indexBytes;
	vkUIVertex_t    *dstVerts;
	uint32_t        *dstIndexes;
	VkDescriptorSet descriptorSet;
	VkPipeline      pipeline;

	if (!vk.initialized || !input || input->numVertexes == 0 || input->numIndexes == 0)
	{
		return;
	}

	if (!image)
	{
		image = tr.whiteImage;
	}
	if (!image || !image->vkDescriptorSet)
	{
		// texture failed to upload (or hasn't been uploaded yet); skip this
		// batch rather than binding a garbage/null descriptor set.
		return;
	}

	if (vk.bindless)
	{
		VK_Draw2DBatched(input, image, stateBits);
		return;
	}

	numVertexes = input->numVertexes;
	numIndexes  = input->numIndexes;

	// draws address the arena with vertexOffset (in vertices): keep the
	// byte offset a multiple of this vertex type's stride
	vk.vertexArenaOffset = VK_ALIGN_UP(vk.vertexArenaOffset, sizeof(vkUIVertex_t));
	vertexBytes          = (VkDeviceSize)numVertexes * sizeof(vkUIVertex_t);
	indexBytes           = (VkDeviceSize)numIndexes * sizeof(uint32_t);

	if (vk.vertexArenaOffset + vertexBytes > vk.vertexArenaSize ||
	    vk.indexArenaOffset + indexBytes > vk.indexArenaSize)
	{
		// Rate-limited: logging this on every dropped draw would itself grow
		// the on-screen console text, causing more 2D draws and more drops -
		// a feedback loop that never recovers once the arena fills up once.
		static int lastWarnFrame = -1;

		if (backEnd.refdef.time != lastWarnFrame)
		{
			Ren_Print("Vulkan: VK_Draw2D - UI vertex/index arena exhausted this frame, dropping remaining 2D draws\n");
			lastWarnFrame = backEnd.refdef.time;
		}
		return;
	}

	dstVerts = (vkUIVertex_t *)((byte *)vk.vertexArenaMapped + vk.vertexArenaOffset);
	for (i = 0; i < numVertexes; i++)
	{
		dstVerts[i].pos[0]   = input->xyz[i][0];
		dstVerts[i].pos[1]   = input->xyz[i][1];
		dstVerts[i].uv[0]    = input->svars.texcoords[0][i][0];
		dstVerts[i].uv[1]    = input->svars.texcoords[0][i][1];
		dstVerts[i].color[0] = input->svars.colors[i][0];
		dstVerts[i].color[1] = input->svars.colors[i][1];
		dstVerts[i].color[2] = input->svars.colors[i][2];
		dstVerts[i].color[3] = input->svars.colors[i][3];
	}

	dstIndexes = (uint32_t *)((byte *)vk.indexArenaMapped + vk.indexArenaOffset);
	Com_Memcpy(dstIndexes, input->indexes, (size_t)indexBytes);

	pipeline = VK_GetUIPipeline(stateBits);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	VK_BindPipelineCached(pipeline);
	VK_Set2DState(vk.uiPipelineLayout, stateBits);

	descriptorSet = (VkDescriptorSet)image->vkDescriptorSet;
	VK_BindDescriptorSetsCached(vk.uiPipelineLayout, 1, &descriptorSet);

	VK_BindArenaBuffers(vk.vertexArenaBuffer, vk.indexArenaBuffer);
	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)numIndexes, 1, (uint32_t)(vk.indexArenaOffset / sizeof(uint32_t)), (int32_t)(vk.vertexArenaOffset / sizeof(vkUIVertex_t)), 0);

	vk.vertexArenaOffset += vertexBytes;
	vk.indexArenaOffset  += indexBytes;
}

/**
 * @brief Sets the dynamic viewport/scissor for 3D scene draws. Must be
 * called once per view (from SetViewportAndScissor() in tr_backend.c,
 * alongside VK_SetProjectionMatrix) rather than cached per-frame: a single
 * frame may contain multiple 3D views with different viewport rects (e.g.
 * the limbo menu's small player-model preview rendered alongside the main
 * world view), so a "set once per frame" cache would leave later views
 * incorrectly using an earlier view's viewport/scissor.
 */
void VK_SetSceneViewport(int x, int y, int width, int height)
{
	VkViewport viewport;
	VkRect2D   scissor;
	int        flippedY;

	if (!vk.initialized)
	{
		return;
	}

	VK_Flush2D(); // keep the pending 2D batch before this in the command buffer

	// backEnd.viewParms.viewportX/Y use GL's bottom-left-origin convention
	// (y is measured up from the bottom of the window), but Vulkan's
	// viewport/scissor use a top-left-origin convention. This cancels out
	// for full-height views (y=0, height=screenHeight) but must be
	// converted explicitly for partial-height views such as the limbo
	// menu's small player-model preview, otherwise the scissor rect ends
	// up in the wrong place and silently clips the view's geometry away.
	flippedY = (int)vk.sceneExtent.height - (y + height);

	// stashed so VK_SetDepthRange() can reissue the viewport with the
	// same rect but a different depth range.
	vk.lastViewportX        = x;
	vk.lastViewportFlippedY = flippedY;
	vk.lastViewportWidth    = width;
	vk.lastViewportHeight   = height;

	Com_Memset(&viewport, 0, sizeof(viewport));
	viewport.x        = (float)x;
	viewport.y        = (float)flippedY;
	viewport.width    = (float)width;
	viewport.height   = (float)height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(vk.commandBuffer, 0, 1, &viewport);
	vk.viewport2DSet = qfalse;

	Com_Memset(&scissor, 0, sizeof(scissor));
	scissor.offset.x      = x;
	scissor.offset.y      = flippedY;
	scissor.extent.width  = (uint32_t)width;
	scissor.extent.height = (uint32_t)height;
	vkCmdSetScissor(vk.commandBuffer, 0, 1, &scissor);
}

/**
 * @brief Reissues the last scene viewport with a different
 * depth range, pinning sky geometry to the far depth plane (mirroring
 * renderer1's RB_StageIteratorSky glDepthRange(1,1) trick: drawing sky
 * first at maximum depth lets normally-depth-tested opaque surfaces
 * drawn afterwards cover it everywhere they're present).
 */
void VK_SetDepthRange(float minDepth, float maxDepth)
{
	VkViewport viewport;

	if (!vk.initialized)
	{
		return;
	}

	VK_Flush2D(); // keep the pending 2D batch before this in the command buffer

	Com_Memset(&viewport, 0, sizeof(viewport));
	viewport.x        = (float)vk.lastViewportX;
	viewport.y        = (float)vk.lastViewportFlippedY;
	viewport.width    = (float)vk.lastViewportWidth;
	viewport.height   = (float)vk.lastViewportHeight;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	vkCmdSetViewport(vk.commandBuffer, 0, 1, &viewport);
	vk.viewport2DSet = qfalse;
}

/**
 * @brief glClearColor() capture point (see vk_gl_compat.h).
 */
void VK_SetClearColor(float r, float g, float b, float a)
{
	vk.clearColor[0] = r;
	vk.clearColor[1] = g;
	vk.clearColor[2] = b;
	vk.clearColor[3] = a;
}

/**
 * @brief renderer1's glClear(GL_COLOR_BUFFER_BIT) with the
 * last glClearColor(). Clears the last scene viewport rect (GL's scissor
 * box during RB_BeginDrawingView/RB_Hyperspace), or the whole framebuffer
 * when fullFramebuffer is set (RB_DrawBuffer's r_clear, before any view).
 */
void VK_ClearSceneColor(qboolean fullFramebuffer)
{
	VkClearAttachment attachment;
	VkClearRect       rect;

	if (!vk.initialized)
	{
		return;
	}

	VK_Flush2D(); // keep the pending 2D batch before this in the command buffer

	Com_Memset(&attachment, 0, sizeof(attachment));
	attachment.aspectMask                  = VK_IMAGE_ASPECT_COLOR_BIT;
	attachment.colorAttachment             = 0;
	attachment.clearValue.color.float32[0] = vk.clearColor[0];
	attachment.clearValue.color.float32[1] = vk.clearColor[1];
	attachment.clearValue.color.float32[2] = vk.clearColor[2];
	attachment.clearValue.color.float32[3] = vk.clearColor[3];

	Com_Memset(&rect, 0, sizeof(rect));
	if (fullFramebuffer || vk.lastViewportWidth <= 0 || vk.lastViewportHeight <= 0)
	{
		rect.rect.extent = vk.sceneExtent;
	}
	else
	{
		rect.rect.offset.x      = vk.lastViewportX;
		rect.rect.offset.y      = vk.lastViewportFlippedY;
		rect.rect.extent.width  = (uint32_t)vk.lastViewportWidth;
		rect.rect.extent.height = (uint32_t)vk.lastViewportHeight;
	}
	rect.baseArrayLayer = 0;
	rect.layerCount     = 1;

	vkCmdClearAttachments(vk.commandBuffer, 1, &attachment, 1, &rect);
}

/**
 * @brief Clears depth to 1.0 inside the last scene viewport
 * rect, replicating renderer1's per-view glClear(GL_DEPTH_BUFFER_BIT) in
 * RB_BeginDrawingView(). The render pass's loadOp=CLEAR only runs once
 * per frame, so without this a portal/mirror subview's depth survives
 * into the main view: main-view surfaces behind the mirror quad (and
 * elsewhere the subview drew nearer geometry) then depth-test against
 * the subview's values instead of a fresh buffer.
 */
void VK_ClearSceneDepth(void)
{
	VkClearAttachment attachment;
	VkClearRect       rect;

	if (!vk.initialized || vk.lastViewportWidth <= 0 || vk.lastViewportHeight <= 0)
	{
		return;
	}

	VK_Flush2D(); // keep the pending 2D batch before this in the command buffer

	Com_Memset(&attachment, 0, sizeof(attachment));
	attachment.aspectMask                      = VK_IMAGE_ASPECT_DEPTH_BIT | (vk.depthHasStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
	attachment.clearValue.depthStencil.depth   = 1.0f;
	attachment.clearValue.depthStencil.stencil = 0;

	Com_Memset(&rect, 0, sizeof(rect));
	rect.rect.offset.x      = vk.lastViewportX;
	rect.rect.offset.y      = vk.lastViewportFlippedY;
	rect.rect.extent.width  = (uint32_t)vk.lastViewportWidth;
	rect.rect.extent.height = (uint32_t)vk.lastViewportHeight;
	rect.baseArrayLayer     = 0;
	rect.layerCount         = 1;

	vkCmdClearAttachments(vk.commandBuffer, 1, &attachment, 1, &rect);
}

/**
 * @brief True when the batch's shader requests polygon offset (decals,
 * marks); selects a depth-biased pipeline variant.
 */
static qboolean VK_ShaderPolygonOffset(const shaderCommands_t *input)
{
	return (input->shader && input->shader->polygonOffset) ? qtrue : qfalse;
}

/**
 * @brief Maps the shader's collapsed-multitexture env (how texture unit 1
 * combines with unit 0 in renderer1's DrawMultitextured()) to the mode
 * index tr3d_lm.frag.glsl expects: 0=modulate, 1=add, 2=replace, 3=decal.
 * r_lightMap forces GL_REPLACE there too, so the lightmap shows alone.
 */
static float VK_MultitextureEnvMode(const shaderCommands_t *input)
{
	if (r_lightMap->integer)
	{
		return 2.0f;
	}

	switch (input->shader ? input->shader->multitextureEnv : 0)
	{
	case GL_ADD: return 1.0f;
	case GL_REPLACE: return 2.0f;
	case GL_DECAL: return 3.0f;
	default: return 0.0f;
	}
}

/**
 * @brief Fills the 48 push-constant floats shared by every scene draw
 * (matrices, alpha test, texenv mode, fog, clip plane).
 */
static void VK_FillScenePushConstants(float *pushConstants, unsigned long stateBits, float texEnvMode)
{
	Com_Memcpy(pushConstants, vk.mvpMatrix, sizeof(vk.mvpMatrix));
	Com_Memcpy(pushConstants + 16, vk.modelMatrix, sizeof(vk.modelMatrix));
	pushConstants[32] = VK_AlphaTestMode(stateBits);
	pushConstants[33] = texEnvMode;
	pushConstants[34] = vk.fogExp ? 1.0f : 0.0f;
	pushConstants[35] = vk.fogDensity;
	pushConstants[36] = vk.fogColor[0];
	pushConstants[37] = vk.fogColor[1];
	pushConstants[38] = vk.fogColor[2];
	pushConstants[39] = vk.fogColor[3];
	pushConstants[40] = vk.fogEnabled ? 1.0f : 0.0f;
	pushConstants[41] = vk.fogStart;
	pushConstants[42] = vk.fogEnd;
	pushConstants[43] = vk.clipPlaneEnabled ? 1.0f : 0.0f;
	pushConstants[44] = vk.clipPlane[0];
	pushConstants[45] = vk.clipPlane[1];
	pushConstants[46] = vk.clipPlane[2];
	pushConstants[47] = vk.clipPlane[3];
}

/**
 * @brief Static world draws: expresses ComputeColors()' result for a stage
 * as color = vertexColor * mul + add (0..1 floats), for the rgbGen/alphaGen
 * modes ComputeVkStatic() accepts. A NULL stage is the constant white of
 * RB_StageIteratorLightmappedMultitexture.
 */
static void VK_StaticStageColor(const shaderStage_t *stage, float *mul, float *add)
{
	float alphaMul = 0.0f, alphaAdd = 1.0f;

	mul[0] = mul[1] = mul[2] = 0.0f;
	add[0] = add[1] = add[2] = 1.0f;

	if (!stage)
	{
		mul[3] = 0.0f;
		add[3] = 1.0f;
		return;
	}

	// rgbGen (also sets the alpha that AGEN_SKIP keeps)
	switch (stage->rgbGen)
	{
	case CGEN_IDENTITY:
		break;
	case CGEN_EXACT_VERTEX:
		mul[0]   = mul[1] = mul[2] = 1.0f;
		add[0]   = add[1] = add[2] = 0.0f;
		alphaMul = 1.0f;
		alphaAdd = 0.0f;
		break;
	case CGEN_VERTEX:
		mul[0]   = mul[1] = mul[2] = tr.identityLight;
		add[0]   = add[1] = add[2] = 0.0f;
		alphaMul = 1.0f;
		alphaAdd = 0.0f;
		break;
	case CGEN_CONST:
		add[0]   = stage->constantColor[0] / 255.0f;
		add[1]   = stage->constantColor[1] / 255.0f;
		add[2]   = stage->constantColor[2] / 255.0f;
		alphaAdd = stage->constantColor[3] / 255.0f;
		break;
	case CGEN_IDENTITY_LIGHTING:
	default:
		add[0]   = add[1] = add[2] = tr.identityLightByte / 255.0f;
		alphaAdd = tr.identityLightByte / 255.0f;
		break;
	}

	switch (stage->alphaGen)
	{
	case AGEN_IDENTITY:
		// ComputeColors() keeps the vertex alpha for rgbGen vertex at identityLight 1
		if (stage->rgbGen != CGEN_IDENTITY && !(stage->rgbGen == CGEN_VERTEX && tr.identityLight == 1.0f))
		{
			alphaMul = 0.0f;
			alphaAdd = 1.0f;
		}
		break;
	case AGEN_CONST:
		alphaMul = 0.0f;
		alphaAdd = stage->constantColor[3] / 255.0f;
		break;
	case AGEN_VERTEX:
		alphaMul = 1.0f;
		alphaAdd = 0.0f;
		break;
	case AGEN_SKIP:
	default:
		break;
	}

	mul[3] = alphaMul;
	add[3] = alphaAdd;
}

/**
 * @brief Reserves count indexes in this frame's scene index arena for a
 * static world batch to write into directly (RB_SurfaceTrianglesStatic).
 * Consecutive calls return consecutive ranges until another draw uses the
 * arena.
 * @param[out] firstIndex position of the range, for vkCmdDrawIndexed
 * @return write pointer, or NULL if the arena is exhausted this frame
 */
glIndex_t *VK_AllocSceneIndexes(int count, int *firstIndex)
{
	VkDeviceSize indexBytes = (VkDeviceSize)count * sizeof(uint32_t);
	glIndex_t    *dst;

	if (!vk.initialized || vk.sceneIndexArenaOffset + indexBytes > vk.sceneIndexArenaSize)
	{
		static int lastWarnFrame = -1;

		if (vk.initialized && backEnd.refdef.time != lastWarnFrame)
		{
			Ren_Print("Vulkan: VK_AllocSceneIndexes - scene index arena exhausted this frame, dropping static world surfaces\n");
			lastWarnFrame = backEnd.refdef.time;
		}
		return NULL;
	}

	dst                       = (glIndex_t *)((byte *)vk.sceneIndexArenaMapped + vk.sceneIndexArenaOffset);
	*firstIndex               = (int)(vk.sceneIndexArenaOffset / sizeof(uint32_t));
	vk.sceneIndexArenaOffset += indexBytes;

	return dst;
}

/**
 * @brief Records the pending merged model batch: per key (pipeline + view
 * state, in first-use order) one multi-draw indirect call; writes the
 * draws' commands and per-draw data into this frame slot's model batch
 * buffers (CPU-written, unlike the world batch's gathered commands).
 */
static void VK_FlushModelBatch(void)
{
	VkDescriptorSet sets[3];
	int             k, d;
	unsigned int    slotBase;

	if (!vk.mbNumDraws || vk.mbFlushing)
	{
		return;
	}
	vk.mbFlushing = qtrue;

	slotBase = (unsigned int)(vk.frameIndex * VK_MB_SLOT_DRAWS);
	sets[0]  = vk.bindlessSet;
	sets[1]  = vk.modelSet;
	sets[2]  = vk.boneSet;

	for (k = 0; k < vk.mbNumKeys; k++)
	{
		unsigned int first = slotBase + (unsigned int)vk.mbFrameDraws, count = 0;

		for (d = 0; d < vk.mbNumDraws; d++)
		{
			if (vk.mbDraws[d].key == k)
			{
				unsigned int slot = slotBase + (unsigned int)vk.mbFrameDraws++;
				unsigned int *cmd = vk.modelCmdMapped + slot * 5;

				cmd[0]                   = vk.mbDraws[d].numIndexes;
				cmd[1]                   = 1;
				cmd[2]                   = vk.mbDraws[d].firstIndex;
				cmd[3]                   = 0;
				cmd[4]                   = slot;
				vk.modelDrawMapped[slot] = vk.mbDraws[d].data;
				count++;
			}
		}

		VK_BindPipelineCached(vk.mbKeys[k].pipeline);
		VK_PushConstantsCached(vk.sceneMD3BLPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		                       vk.mbKeys[k].header, sizeof(vk.mbKeys[k].header));
		if (k == 0)
		{
			// set 2 (the mesh normal tables) has a dynamic offset: bind
			// directly, the next cached bind rebinds
			vkCmdBindDescriptorSets(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.sceneMD3BLPipelineLayout, 0, 3, sets, 1,
			                        &vk.meshTablesOffset);
			vk.boundSetLayout = VK_NULL_HANDLE;
			vk.boundSetCount  = 0;
		}
		VK_BindArenaBuffers(vk.mbKeys[k].vertexBuffer, vk.mbKeys[k].indexBuffer);
		vkCmdDrawIndexedIndirect(vk.commandBuffer, vk.modelCmdBuffer, (VkDeviceSize)first * 5 * sizeof(unsigned int), count,
		                         5 * sizeof(unsigned int));
	}

	vk.mbNumKeys  = 0;
	vk.mbNumDraws = 0;
	vk.mbFlushing = qfalse;
}

/**
 * @brief Adds a GPU-animated model stage draw - MD3/MDC (tess.vkMesh,
 * tr3d_md3_bl.vert reads the frames from the mesh buffer) or MDM
 * (tess.vkSkin, tr3d_skin_bl.vert reads the bones from the bone arena) - to
 * the pending merged model batch (r_vkModelBatch): what VK_Draw3DMesh() /
 * VK_Draw3DSkinned() would push and bind goes into the draw's per-draw data
 * (ofs/ofs2x: its vkModelDraw_t offsets). Draws with the same pipeline,
 * buffers and view state (push constants without the per-entity matrices,
 * lighting and lerp) share a multi-draw indirect call. Only for shaders
 * whose first stage is opaque and writes depth, so the order between
 * entities doesn't matter; a batch's later stage only joins its previous
 * stage's key or a later one.
 * @return qfalse if the draw must be recorded on its own
 */
static qboolean VK_BatchModelDraw(shaderCommands_t *input, VkPipeline pipeline, const float *pushConstants, const image_t *image,
                                  VkBuffer vertexBuffer, VkBuffer indexBuffer, uint32_t firstIndex, const uint32_t *ofs, uint32_t ofs2x)
{
	const shaderStage_t *stage0 = input->shader ? input->shader->stages[0] : NULL;
	vkModelDraw_t       *data;
	float               header[48];
	int                 k;

	if (!image || !image->vkBindlessSlot || !stage0 || input->shader->sort != SS_OPAQUE ||
	    (stage0->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) || !(stage0->stateBits & GLS_DEPTHMASK_TRUE) ||
	    vk.mbFrameDraws + vk.mbNumDraws >= VK_MB_SLOT_DRAWS)
	{
		return qfalse;
	}

	// keep pending 2D and world batches before this in the command buffer
	VK_FlushWorldBatch();
	VK_Flush2DBatch();

	if (vk.mbNumDraws >= VK_MB_MAX_PENDING)
	{
		VK_FlushModelBatch();
	}

	// view state only: the matrices and the env flag are per draw
	Com_Memcpy(header, pushConstants, sizeof(header));
	Com_Memset(header, 0, 32 * sizeof(float));
	header[33] = 0.0f;

	if (input->vkSurfaceSerial != vk.mbSurfaceSerial || !vk.mbNumDraws)
	{
		vk.mbSurfaceSerial = input->vkSurfaceSerial;
		vk.mbMinKey        = 0;
	}

	for (k = vk.mbMinKey; k < vk.mbNumKeys; k++)
	{
		if (vk.mbKeys[k].pipeline == pipeline && vk.mbKeys[k].vertexBuffer == vertexBuffer && vk.mbKeys[k].indexBuffer == indexBuffer &&
		    !memcmp(vk.mbKeys[k].header, header, sizeof(header)))
		{
			break;
		}
	}
	if (k == vk.mbNumKeys)
	{
		if (vk.mbNumKeys == VK_MB_MAX_KEYS)
		{
			VK_FlushModelBatch();
			k = 0;
		}
		vk.mbKeys[k].pipeline     = pipeline;
		vk.mbKeys[k].vertexBuffer = vertexBuffer;
		vk.mbKeys[k].indexBuffer  = indexBuffer;
		Com_Memcpy(vk.mbKeys[k].header, header, sizeof(header));
		vk.mbNumKeys = k + 1;
	}
	vk.mbMinKey = k;

	vk.mbDraws[vk.mbNumDraws].key        = k;
	vk.mbDraws[vk.mbNumDraws].numIndexes = (unsigned int)input->numIndexes;
	vk.mbDraws[vk.mbNumDraws].firstIndex = firstIndex;
	data                                 = &vk.mbDraws[vk.mbNumDraws].data;
	Com_Memcpy(data->mvp, pushConstants, 32 * sizeof(float));       // mvp, modelView
	Com_Memcpy(data->alphaTest, pushConstants + 32, 4 * sizeof(float));
	Com_Memcpy(data->lightDir, pushConstants + 48, 16 * sizeof(float)); // lightDir, ambient, directed, lerp
	Com_Memcpy(data->env, input->vkEnvParams, sizeof(data->env));
	Com_Memcpy(data->ofs, ofs, sizeof(data->ofs));
	data->ofs2[0] = ofs2x;
	data->ofs2[1] = (uint32_t)(image->vkBindlessSlot - 1);
	data->ofs2[2] = 0;
	data->ofs2[3] = 0;
	vk.mbNumDraws++;

	return qtrue;
}

/**
 * @brief Merged model batches (VK_BatchModelDraw): the per-draw data set
 * (draw data, the mesh buffer as storage), the pipeline layout and the
 * per-frame command and data buffers. Optional (needs vk.bindless and
 * multi-draw indirect); without it every model draw is recorded on its own.
 */
static void VK_CreateModelBatchResources(void)
{
	VkDescriptorSetLayoutBinding    bindings[3];
	VkDescriptorSetLayoutCreateInfo setLayoutInfo;
	VkDescriptorPoolSize            poolSize;
	VkDescriptorPoolCreateInfo      poolInfo;
	VkDescriptorSetAllocateInfo     allocInfo;
	VkDescriptorSetLayout           setLayouts[3];
	VkPushConstantRange             pushRange;
	VkPipelineLayoutCreateInfo      layoutInfo;
	VkDescriptorBufferInfo          infos[3];
	VkWriteDescriptorSet            writes[3];
	void                            *mapped;
	int                             i;

	if (!vk.bindless || !vk.multiDrawIndirect || vk.meshBuffer == VK_NULL_HANDLE || vk.boneArenaBuffer == VK_NULL_HANDLE ||
	    vk.sceneBLFragModule == VK_NULL_HANDLE)
	{
		return;
	}

	// 0: per-draw data, 1: mesh buffer (tr3d_md3_bl.vert), 2: bone arena (tr3d_skin_bl.vert)
	Com_Memset(bindings, 0, sizeof(bindings));
	for (i = 0; i < 3; i++)
	{
		bindings[i].binding         = i;
		bindings[i].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[i].descriptorCount = 1;
		bindings[i].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
	}

	Com_Memset(&setLayoutInfo, 0, sizeof(setLayoutInfo));
	setLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setLayoutInfo.bindingCount = 3;
	setLayoutInfo.pBindings    = bindings;
	if (!VK_CheckResult(vkCreateDescriptorSetLayout(vk.device, &setLayoutInfo, NULL, &vk.modelSetLayout), "vkCreateDescriptorSetLayout(model batches)"))
	{
		return;
	}

	Com_Memset(&poolSize, 0, sizeof(poolSize));
	poolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSize.descriptorCount = 3;

	Com_Memset(&poolInfo, 0, sizeof(poolInfo));
	poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes    = &poolSize;
	poolInfo.maxSets       = 1;
	if (!VK_CheckResult(vkCreateDescriptorPool(vk.device, &poolInfo, NULL, &vk.modelPool), "vkCreateDescriptorPool(model batches)"))
	{
		return;
	}

	Com_Memset(&allocInfo, 0, sizeof(allocInfo));
	allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool     = vk.modelPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts        = &vk.modelSetLayout;
	if (!VK_CheckResult(vkAllocateDescriptorSets(vk.device, &allocInfo, &vk.modelSet), "vkAllocateDescriptorSets(model batches)"))
	{
		return;
	}

	Com_Memset(&pushRange, 0, sizeof(pushRange));
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushRange.size       = VK_SCENE_PUSH_FLOATS * sizeof(float);

	setLayouts[0] = vk.bindlessSetLayout;
	setLayouts[1] = vk.modelSetLayout;
	setLayouts[2] = vk.boneSetLayout;

	Com_Memset(&layoutInfo, 0, sizeof(layoutInfo));
	layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount         = 3;
	layoutInfo.pSetLayouts            = setLayouts;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges    = &pushRange;
	if (!VK_CheckResult(vkCreatePipelineLayout(vk.device, &layoutInfo, NULL, &vk.sceneMD3BLPipelineLayout), "vkCreatePipelineLayout(model batches)"))
	{
		return;
	}

	if (!VK_CreateBuffer((VkDeviceSize)VK_FRAMES_IN_FLIGHT * VK_MB_SLOT_DRAWS * sizeof(vkModelDraw_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vk.modelDrawBuffer, &vk.modelDrawMemory) ||
	    !VK_CheckResult(vkMapMemory(vk.device, vk.modelDrawMemory, 0, VK_WHOLE_SIZE, 0, &mapped), "vkMapMemory(model draws)"))
	{
		return;
	}
	vk.modelDrawMapped = (vkModelDraw_t *)mapped;

	if (!VK_CreateBuffer((VkDeviceSize)VK_FRAMES_IN_FLIGHT * VK_MB_SLOT_DRAWS * 5 * sizeof(unsigned int), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vk.modelCmdBuffer, &vk.modelCmdMemory) ||
	    !VK_CheckResult(vkMapMemory(vk.device, vk.modelCmdMemory, 0, VK_WHOLE_SIZE, 0, &mapped), "vkMapMemory(model commands)"))
	{
		return;
	}
	vk.modelCmdMapped = (unsigned int *)mapped;

	Com_Memset(infos, 0, sizeof(infos));
	infos[0].buffer = vk.modelDrawBuffer;
	infos[0].range  = VK_WHOLE_SIZE;
	infos[1].buffer = vk.meshBuffer;
	infos[1].range  = VK_WHOLE_SIZE;
	infos[2].buffer = vk.boneArenaBuffer;
	infos[2].range  = VK_WHOLE_SIZE;

	Com_Memset(writes, 0, sizeof(writes));
	for (i = 0; i < 3; i++)
	{
		writes[i].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[i].dstSet          = vk.modelSet;
		writes[i].dstBinding      = i;
		writes[i].descriptorCount = 1;
		writes[i].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[i].pBufferInfo     = &infos[i];
	}
	vkUpdateDescriptorSets(vk.device, 3, writes, 0, NULL);

	vk.sceneMD3BLVertModule  = VK_CreateShaderModule(tr3d_md3_bl_vert_spv, tr3d_md3_bl_vert_spv_SIZE);
	vk.sceneSkinBLVertModule = VK_CreateShaderModule(tr3d_skin_bl_vert_spv, tr3d_skin_bl_vert_spv_SIZE);
	vk.modelBatch            = (vk.sceneMD3BLVertModule != VK_NULL_HANDLE && vk.sceneSkinBLVertModule != VK_NULL_HANDLE) ? qtrue : qfalse;
}

/**
 * @brief Records the pending merged world batch: per key (pipeline + view
 * state, in first-use order) one multi-draw indirect call over its draws'
 * slots; writes each slot's group command source (for the gather pass) and
 * per-draw data.
 */
static void VK_FlushWorldBatch(void)
{
	VkDescriptorSet sets[2];
	int             k, d;
	unsigned int    slotBase;

	if (!vk.wbNumDraws || vk.wbFlushing)
	{
		return;
	}
	vk.wbFlushing = qtrue;

	slotBase = (unsigned int)(vk.frameIndex * vk.cullMaxBatchDraws);
	sets[0]  = vk.bindlessSet;
	sets[1]  = vk.worldDrawSet;

	for (k = 0; k < vk.wbNumKeys; k++)
	{
		unsigned int first = slotBase + (unsigned int)vk.wbFrameDraws, count = 0;

		for (d = 0; d < vk.wbNumDraws; d++)
		{
			if (vk.wbDraws[d].key == k)
			{
				unsigned int slot = slotBase + (unsigned int)vk.wbFrameDraws++;

				vk.cullGatherMapped[slot]    = vk.wbDraws[d].srcWord;
				vk.cullWorldDrawMapped[slot] = vk.wbDraws[d].data;
				count++;
			}
		}

		VK_BindPipelineCached(vk.wbKeys[k].pipeline);
		VK_PushConstantsCached(vk.sceneBindlessPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		                       vk.wbKeys[k].header, sizeof(vk.wbKeys[k].header));
		VK_BindDescriptorSetsCached(vk.sceneBindlessPipelineLayout, 2, sets);
		VK_BindArenaBuffers(vk.staticWorldBuffer, vk.cullOutBuffer);
		vkCmdDrawIndexedIndirect(vk.commandBuffer, vk.cullBatchCmdBuffer, (VkDeviceSize)first * 5 * sizeof(unsigned int), count,
		                         5 * sizeof(unsigned int));
	}

	vk.wbNumKeys  = 0;
	vk.wbNumDraws = 0;
	vk.wbFlushing = qfalse;
}

/**
 * @brief Adds a GPU-culled group's stage draw to the pending merged world
 * batch instead of recording it (r_vkWorldBatch): same pipeline and view
 * state (push constants up to the stage colors) as a pending key - the
 * draws become one multi-draw indirect call in VK_FlushWorldBatch(). A
 * group's later stage only joins its previous stage's key or a later one,
 * so its stages keep their order. Opaque groups don't overlap except for
 * coplanar surfaces, so the order between groups doesn't matter.
 * @return qfalse if the draw must be recorded on its own
 */
static qboolean VK_BatchWorldDraw(shaderCommands_t *input, VkPipeline pipeline, const float *pushConstants,
                                  const image_t *image0, const image_t *image1, const float *fog)
{
	vkWorldDraw_t *data;
	int           k;

	if (!image0 || !image0->vkBindlessSlot || (image1 && !image1->vkBindlessSlot) ||
	    vk.wbFrameDraws + vk.wbNumDraws >= vk.cullMaxBatchDraws)
	{
		return qfalse;
	}

	// keep pending model and 2D batches before this in the command buffer
	VK_FlushModelBatch();
	VK_Flush2DBatch();

	if (vk.wbNumDraws >= VK_WB_MAX_DRAWS)
	{
		VK_FlushWorldBatch();
	}

	if (input->vkIndirectOffset != vk.wbGroupOffset || !vk.wbNumDraws)
	{
		vk.wbGroupOffset = input->vkIndirectOffset;
		vk.wbMinKey      = 0;
	}

	for (k = vk.wbMinKey; k < vk.wbNumKeys; k++)
	{
		if (vk.wbKeys[k].pipeline == pipeline && !memcmp(vk.wbKeys[k].header, pushConstants, sizeof(vk.wbKeys[k].header)))
		{
			break;
		}
	}
	if (k == vk.wbNumKeys)
	{
		if (vk.wbNumKeys == VK_WB_MAX_KEYS)
		{
			VK_FlushWorldBatch();
			k = 0;
		}
		vk.wbKeys[k].pipeline = pipeline;
		Com_Memcpy(vk.wbKeys[k].header, pushConstants, sizeof(vk.wbKeys[k].header));
		vk.wbNumKeys = k + 1;
	}
	vk.wbMinKey = k;

	vk.wbDraws[vk.wbNumDraws].key     = k;
	vk.wbDraws[vk.wbNumDraws].srcWord = input->vkIndirectOffset / sizeof(unsigned int);
	data                              = &vk.wbDraws[vk.wbNumDraws].data;
	data->textures[0]                 = (uint32_t)(image0->vkBindlessSlot - 1);
	data->textures[1]                 = image1 ? (uint32_t)(image1->vkBindlessSlot - 1) : 0;
	data->textures[2]                 = 0;
	data->textures[3]                 = 0;
	Com_Memcpy(data->colorMul, pushConstants + 48, 4 * sizeof(float));
	Com_Memcpy(data->colorAdd, pushConstants + 52, 4 * sizeof(float));
	if (fog)
	{
		Com_Memcpy(data->fogS, fog, 12 * sizeof(float)); // fogS, fogT, fogEye
	}
	vk.wbNumDraws++;

	return qtrue;
}

/**
 * @brief True if GPU-culled groups can be drawn as merged world batches -
 * needed for fogged groups, whose fog pass only exists there
 * (VK_Draw3DStaticFog)
 */
qboolean VK_WorldBatchOn(void)
{
	return (vk.cullMaxBatchDraws && r_vkWorldBatch->integer) ? qtrue : qfalse;
}

/**
 * @brief RB_FogPass() for a GPU-culled world group (tess.vkIndirect): one
 * more merged world batch draw with the fog image, the fog color (stage
 * color: mul 0, add color) and the texcoord parameters (sVec, tVec, eyeT,
 * RB_CalcFogTexCoordParams) that tr3d_static_fog_bl.vert evaluates per
 * vertex.
 */
void VK_Draw3DStaticFog(shaderCommands_t *input, const image_t *fogImage, unsigned long stateBits, int cullType,
                        const vec4_t sVec, const vec4_t tVec, float eyeT, const byte *color)
{
	float      pushConstants[56];
	float      fog[12];
	VkPipeline pipeline;
	int        i;

	if (!vk.initialized || !input->vkIndirect || !VK_WorldBatchOn())
	{
		return;
	}

	VK_FillScenePushConstants(pushConstants, stateBits, 0.0f);
	for (i = 0; i < 4; i++)
	{
		pushConstants[48 + i] = 0.0f;
		pushConstants[52 + i] = color[i] / 255.0f;
		fog[i]                = sVec[i];
		fog[4 + i]            = tVec[i];
		fog[8 + i]            = 0.0f;
	}
	fog[8] = eyeT;

	pipeline = VK_GetScenePipelineEx(stateBits, VK_MirrorCullType(cullType), qfalse, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE,
	                                 VK_VERTS_STATIC_FOG_BL);
	if (pipeline == VK_NULL_HANDLE || !VK_BatchWorldDraw(input, pipeline, pushConstants, fogImage, NULL, fog))
	{
		static int lastWarnFrame = -1;

		if (backEnd.refdef.time != lastWarnFrame)
		{
			Ren_Print("Vulkan: VK_Draw3DStaticFog - no room in the world batch, dropping fog passes\n");
			lastWarnFrame = backEnd.refdef.time;
		}
	}
}

/**
 * @brief Static world batch (tess.vkStatic): its indexes already point into
 * the static world buffer and were written to the scene index arena while
 * the batch was built (input->vkStaticFirstIndex), so every stage draws that
 * range without copying; colors and texcoords come from the static vertex
 * shaders (see VK_StaticStageColor()).
 */
static void VK_Draw3DStatic(shaderCommands_t *input, int numIndexes, VkDescriptorSet *sets, int numSets,
                            const image_t *image0, const image_t *image1, unsigned long stateBits, int cullType)
{
	qboolean         lightmapped = (numSets == 2) ? qtrue : qfalse;
	int              vertexSource;
	float            pushConstants[56]; // 48 + colorMul + colorAdd
	VkPipeline       pipeline;
	VkPipelineLayout layout;
	unsigned int     dlightOffset = 0;

	if (vk.staticWorldBuffer == VK_NULL_HANDLE || numIndexes == 0)
	{
		return;
	}

	if (!lightmapped && input->vkStage && input->vkStage->bundle[0].tcGen == TCGEN_LIGHTMAP)
	{
		vertexSource = VK_VERTS_STATIC_LIGHTMAP;
	}
	else
	{
		vertexSource = VK_VERTS_STATIC;
	}
	layout = lightmapped ? vk.sceneLMPipelineLayout : vk.scenePipelineLayout;

	// dlit lightmapped batch (RB_StaticDlightsOk): fold renderer1's dlight
	// pass into this draw. The block lists the batch's dlights (origins in
	// the current entity's space, R_TransformDlights) for the vertex shader.
	if (lightmapped && RB_ShaderTakesDlights(input->dlightBits))
	{
		float *dst;
		int   l, count = 0;

		for (l = 0; l < backEnd.refdef.num_dlights; l++)
		{
			if (input->dlightBits & (1 << l))
			{
				count++;
			}
		}

		dst = VK_AllocUniforms((VkDeviceSize)(4 + count * 12) * sizeof(float), &dlightOffset);
		if (!dst)
		{
			return;
		}

		dst[0] = (float)count;
		dst[1] = (r_dynamicLight->integer == 2) ? 2.0f : 1.0f;
		dst[2] = dst[3] = 0.0f;
		dst   += 4;
		for (l = 0; l < backEnd.refdef.num_dlights; l++)
		{
			const dlight_t *dl = &backEnd.refdef.dlights[l];

			if (!(input->dlightBits & (1 << l)))
			{
				continue;
			}

			dst[0] = dl->transformed[0];
			dst[1] = dl->transformed[1];
			dst[2] = dl->transformed[2];
			dst[3] = dl->radius;
			dst[4] = dl->color[0] * 255.0f;
			dst[5] = dl->color[1] * 255.0f;
			dst[6] = dl->color[2] * 255.0f;
			dst[7] = dl->intensity;
			dst[8] = dl->radiusInverseCubed;
			dst[9] = dst[10] = dst[11] = 0.0f;
			dst   += 12;
		}

		vertexSource = VK_VERTS_STATIC_DLIGHT;
		layout       = vk.staticDlightPipelineLayout;
	}

	VK_FillScenePushConstants(pushConstants, stateBits, lightmapped ? VK_MultitextureEnvMode(input) : 0.0f);
	VK_StaticStageColor(input->vkStage, pushConstants + 48, pushConstants + 52);

	// GPU-culled group: merge into a multi-draw (bindless textures)
	if (input->vkIndirect && vk.cullMaxBatchDraws && r_vkWorldBatch->integer && vertexSource != VK_VERTS_STATIC_DLIGHT)
	{
		pipeline = VK_GetScenePipelineEx(stateBits, VK_MirrorCullType(cullType), lightmapped, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE,
		                                 (vertexSource == VK_VERTS_STATIC_LIGHTMAP) ? VK_VERTS_STATIC_LIGHTMAP_BL : VK_VERTS_STATIC_BL);
		if (pipeline != VK_NULL_HANDLE && VK_BatchWorldDraw(input, pipeline, pushConstants, image0, lightmapped ? image1 : NULL, NULL))
		{
			return;
		}
	}

	pipeline = VK_GetScenePipelineEx(stateBits, VK_MirrorCullType(cullType), lightmapped, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE, vertexSource);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	VK_BindPipelineCached(pipeline);
	VK_PushConstantsCached(layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants, sizeof(pushConstants));

	if (vertexSource == VK_VERTS_STATIC_DLIGHT)
	{
		VkDescriptorSet dlSets[3];

		// set 2 has a dynamic offset: bind directly, the next cached bind rebinds
		dlSets[0] = sets[0];
		dlSets[1] = sets[1];
		dlSets[2] = vk.boneSet;
		vkCmdBindDescriptorSets(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 3, dlSets, 1, &dlightOffset);
		vk.boundSetLayout = VK_NULL_HANDLE;
		vk.boundSetCount  = 0;
	}
	else
	{
		VK_BindDescriptorSetsCached(layout, numSets, sets);
	}

	if (input->vkIndirect)
	{
		// GPU-culled group: the culling dispatch wrote its indexes and count
		VK_BindArenaBuffers(vk.staticWorldBuffer, vk.cullOutBuffer);
		vkCmdDrawIndexedIndirect(vk.commandBuffer, vk.cullArenaBuffer, input->vkIndirectOffset, 1, 5 * sizeof(unsigned int));
		return;
	}

	VK_BindArenaBuffers(vk.staticWorldBuffer, vk.sceneIndexArenaBuffer);
	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)numIndexes, 1, (uint32_t)input->vkStaticFirstIndex, 0, 0);
}

/**
 * @brief Reserves room for count foliage instances (RB_SurfaceFoliage) in
 * this frame's scene vertex arena; VK_CommitFoliageInstances() keeps the
 * ones written. Consecutive reservations are contiguous until another draw
 * uses the arena.
 * @param[out] firstInstance position of the first one, for vkCmdDrawIndexed
 * @return write pointer, or NULL if the arena is exhausted this frame
 */
foliageInstance_t *VK_AllocFoliageInstances(int count, int *firstInstance)
{
	VkDeviceSize bytes = (VkDeviceSize)count * sizeof(foliageInstance_t);

	vk.sceneVertexArenaOffset = VK_ALIGN_UP(vk.sceneVertexArenaOffset, sizeof(foliageInstance_t));
	if (!vk.initialized || vk.sceneVertexArenaOffset + bytes > vk.sceneVertexArenaSize)
	{
		static int lastWarnFrame = -1;

		if (vk.initialized && backEnd.refdef.time != lastWarnFrame)
		{
			Ren_Print("Vulkan: VK_AllocFoliageInstances - scene vertex arena exhausted this frame, dropping foliage\n");
			lastWarnFrame = backEnd.refdef.time;
		}
		return NULL;
	}

	*firstInstance = (int)(vk.sceneVertexArenaOffset / sizeof(foliageInstance_t));
	return (foliageInstance_t *)((byte *)vk.sceneVertexArenaMapped + vk.sceneVertexArenaOffset);
}

/**
 * @brief Keeps the first count instances of the last
 * VK_AllocFoliageInstances() reservation.
 */
void VK_CommitFoliageInstances(int count)
{
	vk.sceneVertexArenaOffset += (VkDeviceSize)count * sizeof(foliageInstance_t);
}

/**
 * @brief Records an instanced foliage batch's draws (tess.vkFoliage) with
 * the bound pipeline and push constants: the models from the static world
 * buffer, their indexes in the scene index arena, the instances in the
 * scene vertex arena.
 */
static void VK_DrawFoliageInstances(const shaderCommands_t *input)
{
	VkDeviceSize zero = 0;
	int          i;

	VK_BindArenaBuffers(vk.staticWorldBuffer, vk.sceneIndexArenaBuffer);
	vkCmdBindVertexBuffers(vk.commandBuffer, 1, 1, &vk.sceneVertexArenaBuffer, &zero);

	for (i = 0; i < input->vkNumFoliageDraws; i++)
	{
		const vkFoliageDraw_t *d = &input->vkFoliageDraws[i];

		vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)d->numIndexes, (uint32_t)d->numInstances, (uint32_t)d->firstIndex, 0,
		                 (uint32_t)d->firstInstance);
	}
}

/**
 * @brief A stage of an instanced foliage batch (tess.vkFoliage,
 * RB_SurfaceFoliage): the stage's color from the instance color as for
 * static world batches (VK_StaticStageColor), one instanced draw per model.
 */
static void VK_Draw3DFoliage(shaderCommands_t *input, const image_t *image, unsigned long stateBits, int cullType)
{
	float           pushConstants[56]; // 48 + colorMul + colorAdd
	VkPipeline      pipeline;
	VkDescriptorSet descriptorSet;

	if (vk.staticWorldBuffer == VK_NULL_HANDLE || !input->vkNumFoliageDraws)
	{
		return;
	}

	pipeline = VK_GetScenePipelineEx(stateBits, VK_MirrorCullType(cullType), qfalse, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE,
	                                 VK_VERTS_FOLIAGE);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	VK_FillScenePushConstants(pushConstants, stateBits, 0.0f);
	VK_StaticStageColor(input->vkStage, pushConstants + 48, pushConstants + 52);

	VK_BindPipelineCached(pipeline);
	VK_PushConstantsCached(vk.scenePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants, sizeof(pushConstants));
	descriptorSet = (VkDescriptorSet)image->vkDescriptorSet;
	VK_BindDescriptorSetsCached(vk.scenePipelineLayout, 1, &descriptorSet);

	VK_DrawFoliageInstances(input);
}

/**
 * @brief RB_FogPass() for an instanced foliage batch (tess.vkFoliage): the
 * fog image and color with the texcoord parameters (sVec, tVec, eyeT,
 * RB_CalcFogTexCoordParams) that tr3d_foliage_fog.vert evaluates per vertex.
 */
void VK_Draw3DFoliageFog(shaderCommands_t *input, const image_t *fogImage, unsigned long stateBits, int cullType,
                         const vec4_t sVec, const vec4_t tVec, float eyeT, const byte *color)
{
	float           pushConstants[64]; // 48 + color + fogS + fogT + fogEye
	VkPipeline      pipeline;
	VkDescriptorSet descriptorSet;
	int             i;

	if (!vk.initialized || vk.staticWorldBuffer == VK_NULL_HANDLE || !input->vkFoliage || !input->vkNumFoliageDraws ||
	    !fogImage || !fogImage->vkDescriptorSet)
	{
		return;
	}

	pipeline = VK_GetScenePipelineEx(stateBits, VK_MirrorCullType(cullType), qfalse, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE,
	                                 VK_VERTS_FOLIAGE_FOG);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	VK_FillScenePushConstants(pushConstants, stateBits, 0.0f);
	for (i = 0; i < 4; i++)
	{
		pushConstants[48 + i] = color[i] / 255.0f;
		pushConstants[52 + i] = sVec[i];
		pushConstants[56 + i] = tVec[i];
		pushConstants[60 + i] = 0.0f;
	}
	pushConstants[60] = eyeT;

	VK_BindPipelineCached(pipeline);
	VK_PushConstantsCached(vk.scenePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants, sizeof(pushConstants));
	descriptorSet = (VkDescriptorSet)fogImage->vkDescriptorSet;
	VK_BindDescriptorSetsCached(vk.scenePipelineLayout, 1, &descriptorSet);

	VK_DrawFoliageInstances(input);
}

/**
 * @brief GPU-skinned MDM batch (tess.vkSkin, RB_MDM_SurfaceAnimGPU): the
 * stage's color for tr3d_skin.vert, in the diffuse lighting slots (48-59)
 * - rgbGen lightingDiffuse: the entity's lightDir/ambientLight/directedLight,
 *   as tr3d_diffuse.vert;
 * - constant rgb (identity, identityLighting, const, entity): lightDir 0,
 *   so the shader's table lookup yields floor(ambient) = that color.
 * lightDir.w is the stage's constant alpha, as ComputeColors() would set it
 * (shader_t::vkSkinnable limits the stages to these cases).
 */
static void VK_SkinStageColor(const shaderStage_t *stage, float *pushConstants)
{
	const trRefEntity_t *ent    = backEnd.currentEntity;
	byte                rgba[4] = { 255, 255, 255, 255 };
	qboolean            diffuse = qfalse;
	int                 i;

	if (stage)
	{
		switch (stage->rgbGen)
		{
		case CGEN_LIGHTING_DIFFUSE:
			diffuse = qtrue; // the light table's alpha is 0xff
			break;
		case CGEN_IDENTITY:
			break;
		case CGEN_CONST:
			Com_Memcpy(rgba, stage->constantColor, 4);
			break;
		case CGEN_ENTITY:
			Com_Memcpy(rgba, ent->e.shaderRGBA, 4);
			break;
		case CGEN_IDENTITY_LIGHTING:
		default:
			rgba[0] = rgba[1] = rgba[2] = rgba[3] = tr.identityLightByte;
			break;
		}

		switch (stage->alphaGen)
		{
		case AGEN_IDENTITY:
			rgba[3] = 255;
			break;
		case AGEN_CONST:
			rgba[3] = stage->constantColor[3];
			break;
		case AGEN_ENTITY:
			rgba[3] = ent->e.shaderRGBA[3];
			break;
		case AGEN_SKIP:
		default:
			break;
		}
	}

	for (i = 0; i < 3; i++)
	{
		pushConstants[48 + i] = diffuse ? ent->lightDir[i] : 0.0f;
		pushConstants[52 + i] = diffuse ? ent->ambientLight[i] : (float)rgba[i];
		pushConstants[56 + i] = diffuse ? ent->directedLight[i] : 0.0f;
	}
	pushConstants[51] = rgba[3] / 255.0f;
	pushConstants[55] = 0.0f;
	pushConstants[59] = 0.0f;

	// tcGen environment: texcoords from the batch's env params (set 2)
	pushConstants[33] = (stage && stage->bundle[0].tcGen == TCGEN_ENVIRONMENT_MAPPED) ? 1.0f : 0.0f;
}

/**
 * @brief Draws a GPU-skinned MDM batch (tess.vkSkin): vertices from the skin
 * buffer, indexes already in the scene index arena, the surface's bones as
 * set 1 at its dynamic offset in the bone arena.
 */
static void VK_Draw3DSkinned(shaderCommands_t *input, const image_t *image, unsigned long stateBits, int cullType)
{
	VkDescriptorSet descriptorSet = (VkDescriptorSet)image->vkDescriptorSet;
	float           pushConstants[VK_SCENE_PUSH_FLOATS];
	VkPipeline      pipeline;
	VkDescriptorSet sets[3];
	uint32_t        dynOffsets[2];

	if (vk.skinBuffer == VK_NULL_HANDLE || input->numIndexes == 0)
	{
		return;
	}

	VK_FillScenePushConstants(pushConstants, stateBits, 0.0f);
	VK_SkinStageColor(input->vkStage, pushConstants);
	pushConstants[60] = pushConstants[61] = pushConstants[62] = pushConstants[63] = 0.0f; // no frame interpolation

	// merge into a multi-draw (bindless textures, bones read in the shader)
	if (vk.modelBatch && r_vkModelBatch->integer)
	{
		uint32_t ofs[4];

		ofs[0]   = input->vkSkinBoneOffset / (4 * sizeof(float)); // first bone row
		ofs[1]   = ofs[2] = ofs[3] = 0;
		pipeline = VK_GetScenePipelineEx(stateBits, VK_MirrorCullType(cullType), qfalse, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE, VK_VERTS_SKIN_BL);
		if (pipeline != VK_NULL_HANDLE &&
		    VK_BatchModelDraw(input, pipeline, pushConstants, image, vk.skinBuffer, vk.sceneIndexArenaBuffer, (uint32_t)input->vkSkinFirstIndex, ofs, 0))
		{
			return;
		}
	}

	pipeline = VK_GetScenePipelineEx(stateBits, VK_MirrorCullType(cullType), qfalse, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE, VK_VERTS_SKIN);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	VK_BindPipelineCached(pipeline);

	VK_PushConstantsCached(vk.skinPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants, sizeof(pushConstants));

	// sets 1 and 2 have dynamic offsets: bind the sets directly and make
	// the next cached bind rebind
	sets[0]       = descriptorSet;
	sets[1]       = vk.boneSet;
	sets[2]       = vk.boneSet;
	dynOffsets[0] = input->vkSkinBoneOffset;
	dynOffsets[1] = input->vkEnvOffset;
	vkCmdBindDescriptorSets(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.skinPipelineLayout, 0, 3, sets, 2, dynOffsets);
	vk.boundSetLayout = VK_NULL_HANDLE;
	vk.boundSetCount  = 0;

	VK_BindArenaBuffers(vk.skinBuffer, vk.sceneIndexArenaBuffer);
	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)input->numIndexes, 1, (uint32_t)input->vkSkinFirstIndex, 0, 0);
}

/**
 * @brief Draws a GPU-animated MD3/MDC batch (tess.vkMesh,
 * RB_SurfaceMeshGPU/RB_SurfaceCMeshGPU): the surface's frame and oldframe,
 * its texture coords and (MDC) compressed frame and oldframe from the mesh
 * buffer as five vertex streams, its triangles as the index buffer, and the
 * mesh normal tables (uploaded once per frame to the uniform arena) as set 1
 * for tr3d_md3.vert's normal decoding. Stage colors and env params as for
 * GPU skinning.
 */
static void VK_Draw3DMesh(shaderCommands_t *input, const image_t *image, unsigned long stateBits, int cullType)
{
	VkDescriptorSet descriptorSet = (VkDescriptorSet)image->vkDescriptorSet;
	float           pushConstants[VK_SCENE_PUSH_FLOATS];
	VkPipeline      pipeline;
	VkDescriptorSet sets[3];
	uint32_t        dynOffsets[2];
	VkBuffer        buffers[5];
	VkDeviceSize    offsets[5];

	if (vk.meshBuffer == VK_NULL_HANDLE || input->numIndexes == 0)
	{
		return;
	}

	if (vk.meshTablesFrame != vk.frameCount)
	{
		float *dst = VK_AllocUniforms(VK_MESH_TABLE_FLOATS * sizeof(float), &vk.meshTablesOffset);
		int   i;

		if (!dst)
		{
			return;
		}
		for (i = 0; i < VK_MESH_SIN_PAIRS; i++)
		{
			dst[2 * i]     = tr.sinTable[16 * i];
			dst[2 * i + 1] = tr.sinTable[(16 * i - 1) & FUNCTABLE_MASK];
		}
		dst += 2 * VK_MESH_SIN_PAIRS;
		for (i = 0; i < NUMMDCVERTEXNORMALS; i++, dst += 4)
		{
			VectorCopy(r_anormals[i], dst);
			dst[3] = 0.0f;
		}
		vk.meshTablesFrame = vk.frameCount;
	}

	VK_FillScenePushConstants(pushConstants, stateBits, 0.0f);
	VK_SkinStageColor(input->vkStage, pushConstants);
	pushConstants[55] = (float)input->vkMeshFlags;
	pushConstants[60] = input->vkMeshLerp[0];
	pushConstants[61] = input->vkMeshLerp[1];
	pushConstants[62] = input->vkMeshLerp[2];
	pushConstants[63] = input->vkMeshLerp[3];

	// merge into a multi-draw (bindless textures, frames read in the shader)
	if (vk.modelBatch && r_vkModelBatch->integer)
	{
		uint32_t ofs[4];

		ofs[0]   = input->vkMeshNewOfs / sizeof(uint32_t);
		ofs[1]   = input->vkMeshOldOfs / sizeof(uint32_t);
		ofs[2]   = input->vkMeshStOfs / sizeof(uint32_t);
		ofs[3]   = input->vkMeshNewCompOfs / sizeof(uint32_t);
		pipeline = VK_GetScenePipelineEx(stateBits, VK_MirrorCullType(cullType), qfalse, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE, VK_VERTS_MD3_BL);
		if (pipeline != VK_NULL_HANDLE &&
		    VK_BatchModelDraw(input, pipeline, pushConstants, image, vk.meshBuffer, vk.meshBuffer, input->vkMeshIndexOfs / sizeof(uint32_t), ofs,
		                      input->vkMeshOldCompOfs / sizeof(uint32_t)))
		{
			return;
		}
	}

	pipeline = VK_GetScenePipelineEx(stateBits, VK_MirrorCullType(cullType), qfalse, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE, VK_VERTS_MD3);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	VK_BindPipelineCached(pipeline);
	VK_PushConstantsCached(vk.skinPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants, sizeof(pushConstants));

	// sets 1 and 2 have dynamic offsets: bind directly, the next cached
	// bind rebinds
	sets[0]       = descriptorSet;
	sets[1]       = vk.boneSet;
	sets[2]       = vk.boneSet;
	dynOffsets[0] = vk.meshTablesOffset;
	dynOffsets[1] = input->vkEnvOffset;
	vkCmdBindDescriptorSets(vk.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.skinPipelineLayout, 0, 3, sets, 2, dynOffsets);
	vk.boundSetLayout = VK_NULL_HANDLE;
	vk.boundSetCount  = 0;

	buffers[0] = buffers[1] = buffers[2] = buffers[3] = buffers[4] = vk.meshBuffer;
	offsets[0] = input->vkMeshNewOfs;
	offsets[1] = input->vkMeshOldOfs;
	offsets[2] = input->vkMeshStOfs;
	offsets[3] = input->vkMeshNewCompOfs;
	offsets[4] = input->vkMeshOldCompOfs;
	vkCmdBindVertexBuffers(vk.commandBuffer, 0, 5, buffers, offsets);
	vkCmdBindIndexBuffer(vk.commandBuffer, vk.meshBuffer, input->vkMeshIndexOfs, VK_INDEX_TYPE_UINT32);
	vk.boundVertexBuffer = VK_NULL_HANDLE; // the arena binds rebind
	vk.boundIndexBuffer  = VK_NULL_HANDLE;

	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)input->numIndexes, 1, 0, 0, 0);
}

/**
 * @brief rgbGen lightingDiffuse stage (tess.vkDiffuse): like the arena path
 * of VK_Draw3DIndexes(), but the vertices carry the model-space normal and
 * tr3d_diffuse.vert computes renderer1's RB_CalcDiffuseColor() from the
 * current entity's lightDir/ambientLight/directedLight. svars.colors only
 * supplies alpha (ComputeColors() skipped the rgb).
 */
static void VK_Draw3DDiffuse(shaderCommands_t *input, const glIndex_t *indexes, int numIndexes, VkDescriptorSet descriptorSet,
                             unsigned long stateBits, int cullType)
{
	const trRefEntity_t *ent = backEnd.currentEntity;
	int                 numVertexes = input->numVertexes, i;
	VkDeviceSize        vertexBytes, indexBytes;
	vk3DVertexN_t       *dstVerts;
	float               pushConstants[VK_SCENE_PUSH_FLOATS];
	VkPipeline          pipeline;

	pipeline = VK_GetScenePipelineEx(stateBits, VK_MirrorCullType(cullType), qfalse, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE, VK_VERTS_DIFFUSE);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	vk.sceneVertexArenaOffset = VK_ALIGN_UP(vk.sceneVertexArenaOffset, sizeof(vk3DVertexN_t));
	vertexBytes               = (VkDeviceSize)numVertexes * sizeof(vk3DVertexN_t);
	indexBytes                = (VkDeviceSize)numIndexes * sizeof(uint32_t);

	if (vk.sceneVertexArenaOffset + vertexBytes > vk.sceneVertexArenaSize ||
	    vk.sceneIndexArenaOffset + indexBytes > vk.sceneIndexArenaSize)
	{
		static int lastWarnFrame = -1;

		if (backEnd.refdef.time != lastWarnFrame)
		{
			Ren_Print("Vulkan: VK_Draw3DDiffuse - scene vertex/index arena exhausted this frame, dropping remaining 3D draws\n");
			lastWarnFrame = backEnd.refdef.time;
		}
		return;
	}

	dstVerts = (vk3DVertexN_t *)((byte *)vk.sceneVertexArenaMapped + vk.sceneVertexArenaOffset);
	for (i = 0; i < numVertexes; i++)
	{
		dstVerts[i].pos[0]    = input->xyz[i][0];
		dstVerts[i].pos[1]    = input->xyz[i][1];
		dstVerts[i].pos[2]    = input->xyz[i][2];
		dstVerts[i].uv[0]     = input->svars.texcoords[0][i][0];
		dstVerts[i].uv[1]     = input->svars.texcoords[0][i][1];
		dstVerts[i].color[3]  = input->svars.colors[i][3];
		dstVerts[i].normal[0] = input->normal[i][0];
		dstVerts[i].normal[1] = input->normal[i][1];
		dstVerts[i].normal[2] = input->normal[i][2];
	}

	Com_Memcpy((byte *)vk.sceneIndexArenaMapped + vk.sceneIndexArenaOffset, indexes, (size_t)indexBytes);

	VK_BindPipelineCached(pipeline);

	VK_FillScenePushConstants(pushConstants, stateBits, 0.0f);
	pushConstants[48] = ent->lightDir[0];
	pushConstants[49] = ent->lightDir[1];
	pushConstants[50] = ent->lightDir[2];
	pushConstants[51] = 0.0f;
	pushConstants[52] = ent->ambientLight[0];
	pushConstants[53] = ent->ambientLight[1];
	pushConstants[54] = ent->ambientLight[2];
	pushConstants[55] = 0.0f;
	pushConstants[56] = ent->directedLight[0];
	pushConstants[57] = ent->directedLight[1];
	pushConstants[58] = ent->directedLight[2];
	pushConstants[59] = 0.0f;
	VK_PushConstantsCached(vk.scenePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants, sizeof(pushConstants));

	VK_BindDescriptorSetsCached(vk.scenePipelineLayout, 1, &descriptorSet);

	VK_BindArenaBuffers(vk.sceneVertexArenaBuffer, vk.sceneIndexArenaBuffer);
	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)numIndexes, 1, (uint32_t)(vk.sceneIndexArenaOffset / sizeof(uint32_t)), (int32_t)(vk.sceneVertexArenaOffset / sizeof(vk3DVertexN_t)), 0);

	vk.sceneVertexArenaOffset += vertexBytes;
	vk.sceneIndexArenaOffset  += indexBytes;
}

/**
 * @brief Submits one already-tessellated 3D shader stage
 * batch (single texture) using the lazily-created pipeline variant that
 * matches stateBits/cullType. Vertex data is read from input->xyz,
 * input->svars.texcoords[0] and input->svars.colors (populated by
 * ComputeColors()/ComputeTexCoords() in tr_shade.c).
 */
void VK_Draw3D(shaderCommands_t *input, struct image_s *image, unsigned long stateBits, int cullType)
{
	VK_Draw3DIndexes(input, input->indexes, input->numIndexes, image, stateBits, cullType);
}

/**
 * @brief VK_Draw3D() drawing only the given subset of indexes (e.g. the
 * triangles a dynamic light actually touches) while still reading every
 * vertex from input.
 */
void VK_Draw3DIndexes(shaderCommands_t *input, const glIndex_t *indexes, int numIndexes, struct image_s *image, unsigned long stateBits, int cullType)
{
	int             numVertexes, i;
	VkDeviceSize    vertexBytes, indexBytes;
	vk3DVertex_t    *dstVerts;
	uint32_t        *dstIndexes;
	float           pushConstants[48];
	VkPipeline      pipeline;
	VkDescriptorSet descriptorSet;

	if (!vk.initialized || !input || (input->numVertexes == 0 && !input->vkStatic && !input->vkSkin && !input->vkMesh && !input->vkFoliage) ||
	    numIndexes == 0)
	{
		return;
	}

	if (!image)
	{
		image = tr.whiteImage;
	}
	if (!image || !image->vkDescriptorSet)
	{
		return;
	}

	if (input->vkFoliage)
	{
		VK_Draw3DFoliage(input, image, stateBits, cullType);
		return;
	}

	if (input->vkSkin)
	{
		VK_Draw3DSkinned(input, image, stateBits, cullType);
		return;
	}

	if (input->vkMesh)
	{
		VK_Draw3DMesh(input, image, stateBits, cullType);
		return;
	}

	if (input->vkStatic)
	{
		// always the whole batch: dlit surfaces are never static
		descriptorSet = (VkDescriptorSet)image->vkDescriptorSet;
		VK_Draw3DStatic(input, input->numIndexes, &descriptorSet, 1, image, NULL, stateBits, cullType);
		return;
	}

	if (input->vkDiffuse)
	{
		VK_Draw3DDiffuse(input, indexes, numIndexes, (VkDescriptorSet)image->vkDescriptorSet, stateBits, cullType);
		return;
	}

	pipeline = VK_GetScenePipeline(stateBits, VK_MirrorCullType(cullType), qfalse, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	numVertexes = input->numVertexes;

	// draws address the arena with vertexOffset (in vertices): keep the
	// byte offset a multiple of this vertex type's stride
	vk.sceneVertexArenaOffset = VK_ALIGN_UP(vk.sceneVertexArenaOffset, sizeof(vk3DVertex_t));
	vertexBytes               = (VkDeviceSize)numVertexes * sizeof(vk3DVertex_t);
	indexBytes                = (VkDeviceSize)numIndexes * sizeof(uint32_t);

	if (vk.sceneVertexArenaOffset + vertexBytes > vk.sceneVertexArenaSize ||
	    vk.sceneIndexArenaOffset + indexBytes > vk.sceneIndexArenaSize)
	{
		static int lastWarnFrame = -1;

		if (backEnd.refdef.time != lastWarnFrame)
		{
			Ren_Print("Vulkan: VK_Draw3D - scene vertex/index arena exhausted this frame, dropping remaining 3D draws\n");
			lastWarnFrame = backEnd.refdef.time;
		}
		return;
	}

	dstVerts = (vk3DVertex_t *)((byte *)vk.sceneVertexArenaMapped + vk.sceneVertexArenaOffset);
	for (i = 0; i < numVertexes; i++)
	{
		dstVerts[i].pos[0]   = input->xyz[i][0];
		dstVerts[i].pos[1]   = input->xyz[i][1];
		dstVerts[i].pos[2]   = input->xyz[i][2];
		dstVerts[i].uv[0]    = input->svars.texcoords[0][i][0];
		dstVerts[i].uv[1]    = input->svars.texcoords[0][i][1];
		dstVerts[i].color[0] = input->svars.colors[i][0];
		dstVerts[i].color[1] = input->svars.colors[i][1];
		dstVerts[i].color[2] = input->svars.colors[i][2];
		dstVerts[i].color[3] = input->svars.colors[i][3];
	}

	dstIndexes = (uint32_t *)((byte *)vk.sceneIndexArenaMapped + vk.sceneIndexArenaOffset);
	Com_Memcpy(dstIndexes, indexes, (size_t)indexBytes);

	VK_BindPipelineCached(pipeline);

	VK_FillScenePushConstants(pushConstants, stateBits, 0.0f);
	VK_PushConstantsCached(vk.scenePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants, sizeof(pushConstants));

	descriptorSet = (VkDescriptorSet)image->vkDescriptorSet;
	VK_BindDescriptorSetsCached(vk.scenePipelineLayout, 1, &descriptorSet);

	VK_BindArenaBuffers(vk.sceneVertexArenaBuffer, vk.sceneIndexArenaBuffer);
	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)numIndexes, 1, (uint32_t)(vk.sceneIndexArenaOffset / sizeof(uint32_t)), (int32_t)(vk.sceneVertexArenaOffset / sizeof(vk3DVertex_t)), 0);

	vk.sceneVertexArenaOffset += vertexBytes;
	vk.sceneIndexArenaOffset  += indexBytes;
}

void VK_Draw3DTwoStage(shaderCommands_t *input, struct image_s *image0, struct image_s *image1, const byte (*colors0)[4],
                       const byte (*colors1)[4], unsigned long stateBits0, unsigned long blendBits1, int cullType)
{
	int             numVertexes = input->numVertexes, numIndexes = input->numIndexes, i;
	VkDeviceSize    vertexBytes, indexBytes;
	vk3DVertex2_t   *dstVerts;
	float           pushConstants[56]; // 48 + blend1
	VkPipeline      pipeline;
	VkDescriptorSet sets[2];

	if (!vk.initialized || numVertexes == 0 || numIndexes == 0)
	{
		return;
	}
	if (!image0)
	{
		image0 = tr.whiteImage;
	}
	if (!image1)
	{
		image1 = tr.whiteImage;
	}
	if (!image0->vkDescriptorSet || !image1->vkDescriptorSet)
	{
		return;
	}

	pipeline = VK_GetScenePipelineEx(stateBits0, VK_MirrorCullType(cullType), qtrue, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE, VK_VERTS_TWOSTAGE);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	vk.sceneVertexArenaOffset = VK_ALIGN_UP(vk.sceneVertexArenaOffset, sizeof(vk3DVertex2_t));
	vertexBytes               = (VkDeviceSize)numVertexes * sizeof(vk3DVertex2_t);
	indexBytes                = (VkDeviceSize)numIndexes * sizeof(uint32_t);

	if (vk.sceneVertexArenaOffset + vertexBytes > vk.sceneVertexArenaSize ||
	    vk.sceneIndexArenaOffset + indexBytes > vk.sceneIndexArenaSize)
	{
		static int lastWarnFrame = -1;

		if (backEnd.refdef.time != lastWarnFrame)
		{
			Ren_Print("Vulkan: VK_Draw3DTwoStage - scene vertex/index arena exhausted this frame, dropping remaining 3D draws\n");
			lastWarnFrame = backEnd.refdef.time;
		}
		return;
	}

	dstVerts = (vk3DVertex2_t *)((byte *)vk.sceneVertexArenaMapped + vk.sceneVertexArenaOffset);
	for (i = 0; i < numVertexes; i++)
	{
		dstVerts[i].pos[0] = input->xyz[i][0];
		dstVerts[i].pos[1] = input->xyz[i][1];
		dstVerts[i].pos[2] = input->xyz[i][2];
		dstVerts[i].uv0[0] = input->svars.texcoords[0][i][0];
		dstVerts[i].uv0[1] = input->svars.texcoords[0][i][1];
		dstVerts[i].uv1[0] = input->svars.texcoords[1][i][0];
		dstVerts[i].uv1[1] = input->svars.texcoords[1][i][1];
		Com_Memcpy(dstVerts[i].color0, colors0[i], 4);
		Com_Memcpy(dstVerts[i].color1, colors1[i], 4);
	}

	Com_Memcpy((byte *)vk.sceneIndexArenaMapped + vk.sceneIndexArenaOffset, input->indexes, (size_t)indexBytes);

	VK_BindPipelineCached(pipeline);

	VK_FillScenePushConstants(pushConstants, stateBits0, 0.0f);
	pushConstants[48] = (float)(blendBits1 & GLS_SRCBLEND_BITS);
	pushConstants[49] = (float)((blendBits1 & GLS_DSTBLEND_BITS) >> 4);
	pushConstants[50] = 0.0f;
	pushConstants[51] = 0.0f;
	pushConstants[52] = pushConstants[53] = pushConstants[54] = pushConstants[55] = 0.0f;
	VK_PushConstantsCached(vk.sceneLMPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants, sizeof(pushConstants));

	sets[0] = (VkDescriptorSet)image0->vkDescriptorSet;
	sets[1] = (VkDescriptorSet)image1->vkDescriptorSet;
	VK_BindDescriptorSetsCached(vk.sceneLMPipelineLayout, 2, sets);

	VK_BindArenaBuffers(vk.sceneVertexArenaBuffer, vk.sceneIndexArenaBuffer);
	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)numIndexes, 1, (uint32_t)(vk.sceneIndexArenaOffset / sizeof(uint32_t)), (int32_t)(vk.sceneVertexArenaOffset / sizeof(vk3DVertex2_t)), 0);

	vk.sceneVertexArenaOffset += vertexBytes;
	vk.sceneIndexArenaOffset  += indexBytes;
}

/**
 * @brief Submits one already-tessellated 3D shader stage
 * batch modulating two textures (base * lightmap), used by
 * RB_StageIteratorLightmappedMultitexture and 2-stage (base+lightmap)
 * shaders handled by RB_StageIteratorGeneric's 3D path. UVs are read from
 * input->svars.texcoords[0]/[1], color from input->svars.colors.
 */
void VK_Draw3DLightmapped(shaderCommands_t *input, struct image_s *base, struct image_s *lightmap, unsigned long stateBits, int cullType)
{
	int             numVertexes, numIndexes, i;
	VkDeviceSize    vertexBytes, indexBytes;
	vk3DVertexLM_t  *dstVerts;
	uint32_t        *dstIndexes;
	float           pushConstants[48];
	VkPipeline      pipeline;
	VkDescriptorSet descriptorSets[2];

	if (!vk.initialized || !input || (input->numVertexes == 0 && !input->vkStatic) || input->numIndexes == 0)
	{
		return;
	}

	if (!base)
	{
		base = tr.whiteImage;
	}
	if (!lightmap)
	{
		lightmap = tr.whiteImage;
	}
	if (!base->vkDescriptorSet || !lightmap->vkDescriptorSet)
	{
		return;
	}

	if (input->vkStatic)
	{
		descriptorSets[0] = (VkDescriptorSet)base->vkDescriptorSet;
		descriptorSets[1] = (VkDescriptorSet)lightmap->vkDescriptorSet;
		VK_Draw3DStatic(input, input->numIndexes, descriptorSets, 2, base, lightmap, stateBits, cullType);
		return;
	}

	pipeline = VK_GetScenePipeline(stateBits, VK_MirrorCullType(cullType), qtrue, VK_ShaderPolygonOffset(input), VK_SPECIAL_NONE);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	numVertexes = input->numVertexes;
	numIndexes  = input->numIndexes;

	// draws address the arena with vertexOffset (in vertices): keep the
	// byte offset a multiple of this vertex type's stride
	vk.sceneVertexArenaOffset = VK_ALIGN_UP(vk.sceneVertexArenaOffset, sizeof(vk3DVertexLM_t));
	vertexBytes               = (VkDeviceSize)numVertexes * sizeof(vk3DVertexLM_t);
	indexBytes                = (VkDeviceSize)numIndexes * sizeof(uint32_t);

	if (vk.sceneVertexArenaOffset + vertexBytes > vk.sceneVertexArenaSize ||
	    vk.sceneIndexArenaOffset + indexBytes > vk.sceneIndexArenaSize)
	{
		static int lastWarnFrame = -1;

		if (backEnd.refdef.time != lastWarnFrame)
		{
			Ren_Print("Vulkan: VK_Draw3DLightmapped - scene vertex/index arena exhausted this frame, dropping remaining 3D draws\n");
			lastWarnFrame = backEnd.refdef.time;
		}
		return;
	}

	dstVerts = (vk3DVertexLM_t *)((byte *)vk.sceneVertexArenaMapped + vk.sceneVertexArenaOffset);
	for (i = 0; i < numVertexes; i++)
	{
		dstVerts[i].pos[0]   = input->xyz[i][0];
		dstVerts[i].pos[1]   = input->xyz[i][1];
		dstVerts[i].pos[2]   = input->xyz[i][2];
		dstVerts[i].uv0[0]   = input->svars.texcoords[0][i][0];
		dstVerts[i].uv0[1]   = input->svars.texcoords[0][i][1];
		dstVerts[i].uv1[0]   = input->svars.texcoords[1][i][0];
		dstVerts[i].uv1[1]   = input->svars.texcoords[1][i][1];
		dstVerts[i].color[0] = input->svars.colors[i][0];
		dstVerts[i].color[1] = input->svars.colors[i][1];
		dstVerts[i].color[2] = input->svars.colors[i][2];
		dstVerts[i].color[3] = input->svars.colors[i][3];
	}

	dstIndexes = (uint32_t *)((byte *)vk.sceneIndexArenaMapped + vk.sceneIndexArenaOffset);
	Com_Memcpy(dstIndexes, input->indexes, (size_t)indexBytes);

	VK_BindPipelineCached(pipeline);

	VK_FillScenePushConstants(pushConstants, stateBits, VK_MultitextureEnvMode(input));
	VK_PushConstantsCached(vk.sceneLMPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants, sizeof(pushConstants));

	descriptorSets[0] = (VkDescriptorSet)base->vkDescriptorSet;
	descriptorSets[1] = (VkDescriptorSet)lightmap->vkDescriptorSet;
	VK_BindDescriptorSetsCached(vk.sceneLMPipelineLayout, 2, descriptorSets);

	VK_BindArenaBuffers(vk.sceneVertexArenaBuffer, vk.sceneIndexArenaBuffer);
	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)numIndexes, 1, (uint32_t)(vk.sceneIndexArenaOffset / sizeof(uint32_t)), (int32_t)(vk.sceneVertexArenaOffset / sizeof(vk3DVertexLM_t)), 0);

	vk.sceneVertexArenaOffset += vertexBytes;
	vk.sceneIndexArenaOffset  += indexBytes;
}

/**
 * @brief Number of stencil bits of the depth attachment
 * (reported as glConfig.stencilBits; renderer1 code gates stencil
 * shadows on >= 4).
 */
int VK_StencilBits(void)
{
	return vk.depthHasStencil ? 8 : 0;
}

/**
 * @brief Uploads raw positions with one constant color and
 * draws them with the given (special) scene pipeline, white texture.
 */
static void VK_DrawSceneRaw(VkPipeline pipeline, const vec4_t *xyz, int numVertexes, const byte *color,
                            const glIndex_t *indexes, int numIndexes, const float *mvp, const float *modelView, qboolean useClip)
{
	VkDeviceSize    vertexBytes, indexBytes;
	vk3DVertex_t    *dstVerts;
	float           pushConstants[48];
	VkDescriptorSet descriptorSet;
	int             i;

	if (pipeline == VK_NULL_HANDLE || numVertexes == 0 || numIndexes == 0 || !tr.whiteImage || !tr.whiteImage->vkDescriptorSet)
	{
		return;
	}

	// draws address the arena with vertexOffset (in vertices): keep the
	// byte offset a multiple of this vertex type's stride
	vk.sceneVertexArenaOffset = VK_ALIGN_UP(vk.sceneVertexArenaOffset, sizeof(vk3DVertex_t));
	vertexBytes               = (VkDeviceSize)numVertexes * sizeof(vk3DVertex_t);
	indexBytes                = (VkDeviceSize)numIndexes * sizeof(uint32_t);

	if (vk.sceneVertexArenaOffset + vertexBytes > vk.sceneVertexArenaSize ||
	    vk.sceneIndexArenaOffset + indexBytes > vk.sceneIndexArenaSize)
	{
		return;
	}

	dstVerts = (vk3DVertex_t *)((byte *)vk.sceneVertexArenaMapped + vk.sceneVertexArenaOffset);
	for (i = 0; i < numVertexes; i++)
	{
		dstVerts[i].pos[0]   = xyz[i][0];
		dstVerts[i].pos[1]   = xyz[i][1];
		dstVerts[i].pos[2]   = xyz[i][2];
		dstVerts[i].uv[0]    = 0.0f;
		dstVerts[i].uv[1]    = 0.0f;
		dstVerts[i].color[0] = color[0];
		dstVerts[i].color[1] = color[1];
		dstVerts[i].color[2] = color[2];
		dstVerts[i].color[3] = color[3];
	}
	Com_Memcpy((byte *)vk.sceneIndexArenaMapped + vk.sceneIndexArenaOffset, indexes, (size_t)indexBytes);

	VK_BindPipelineCached(pipeline);

	Com_Memset(pushConstants, 0, sizeof(pushConstants));
	Com_Memcpy(pushConstants, mvp, 16 * sizeof(float));
	Com_Memcpy(pushConstants + 16, modelView, 16 * sizeof(float));
	if (useClip && vk.clipPlaneEnabled)
	{
		pushConstants[43] = 1.0f;
		pushConstants[44] = vk.clipPlane[0];
		pushConstants[45] = vk.clipPlane[1];
		pushConstants[46] = vk.clipPlane[2];
		pushConstants[47] = vk.clipPlane[3];
	}
	VK_PushConstantsCached(vk.scenePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants, sizeof(pushConstants));

	descriptorSet = (VkDescriptorSet)tr.whiteImage->vkDescriptorSet;
	VK_BindDescriptorSetsCached(vk.scenePipelineLayout, 1, &descriptorSet);

	VK_BindArenaBuffers(vk.sceneVertexArenaBuffer, vk.sceneIndexArenaBuffer);
	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)numIndexes, 1, (uint32_t)(vk.sceneIndexArenaOffset / sizeof(uint32_t)), (int32_t)(vk.sceneVertexArenaOffset / sizeof(vk3DVertex_t)), 0);

	vk.sceneVertexArenaOffset += vertexBytes;
	vk.sceneIndexArenaOffset  += indexBytes;
}

/**
 * @brief One stencil pass of a shadow volume (renderer1's
 * R_RenderShadowEdges() under glStencilOp(KEEP, KEEP, INCR/DECR) with
 * color writes off). cullType uses the engine's CT_* convention, already
 * resolved for mirrors by the caller.
 */
void VK_DrawShadowVolume(const vec4_t *xyz, int numVertexes, const glIndex_t *indexes, int numIndexes, int cullType, qboolean increment)
{
	static const byte color[4] = { 51, 51, 51, 255 };
	VkPipeline        pipeline;

	if (!vk.initialized || !vk.depthHasStencil)
	{
		return;
	}

	pipeline = VK_GetScenePipeline(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO, cullType, qfalse, qfalse,
	                               increment ? VK_SPECIAL_SHADOW_INCR : VK_SPECIAL_SHADOW_DECR);

	VK_DrawSceneRaw(pipeline, xyz, numVertexes, color, indexes, numIndexes, vk.mvpMatrix, vk.modelMatrix, qtrue);
}

/**
 * @brief renderer1's RB_ShadowFinish() darken pass: multiply
 * the current view by 0.6 wherever the stencil is non-zero. Drawn as a
 * clip-space quad covering the view (identity matrices) without depth
 * test/write; renderer1 instead draws a near eye-space quad with depth
 * writes on, which has the same visible result.
 */
void VK_DrawShadowFinish(void)
{
	static const byte      color[4]     = { 153, 153, 153, 255 };
	static const vec4_t    quad[4]      = { { -1, -1, 0.5f, 1 }, { 1, -1, 0.5f, 1 }, { 1, 1, 0.5f, 1 }, { -1, 1, 0.5f, 1 } };
	static const glIndex_t idx[6]       = { 0, 1, 2, 0, 2, 3 };
	static const float     identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	VkPipeline             pipeline;

	if (!vk.initialized || !vk.depthHasStencil)
	{
		return;
	}

	pipeline = VK_GetScenePipeline(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO, CT_TWO_SIDED, qfalse, qfalse,
	                               VK_SPECIAL_SHADOW_DARKEN);

	VK_DrawSceneRaw(pipeline, quad, 4, color, idx, 6, identity, identity, qfalse);
}

/**
 * @brief Device anisotropy limit (0 if unsupported), reported
 * through renderer1's textureFilterAnisotropic/maxAnisotropy globals.
 */
float VK_GetMaxAnisotropy(void)
{
	return vk.maxSamplerAnisotropy;
}

/**
 * @brief Vulkan side of renderer1's GL_TextureMode(): rebuild
 * the mipmapped-image samplers for the new min/mag filter and rebind every
 * mipmapped image's descriptor to them (the sampler is baked into each
 * combined-image-sampler descriptor). Runs from RE_BeginFrame between
 * frames; waits for the GPU so no in-flight frame still uses the old ones.
 */
void VK_SetTextureMode(int minFilter, int magFilter)
{
	VkSampler oldRepeat, oldClamp;
	int       i;

	if (!vk.initialized || (minFilter == vk.texFilterMin && magFilter == vk.texFilterMag))
	{
		return;
	}

	vkDeviceWaitIdle(vk.device);

	oldRepeat       = vk.mipSamplerRepeat;
	oldClamp        = vk.mipSamplerClamp;
	vk.texFilterMin = minFilter;
	vk.texFilterMag = magFilter;

	if (!VK_CreateMipSamplers())
	{
		Ren_Print("VK_SetTextureMode: sampler creation failed, keeping previous filter\n");
		if (vk.mipSamplerRepeat != VK_NULL_HANDLE)
		{
			vkDestroySampler(vk.device, vk.mipSamplerRepeat, NULL);
		}
		if (vk.mipSamplerClamp != VK_NULL_HANDLE)
		{
			vkDestroySampler(vk.device, vk.mipSamplerClamp, NULL);
		}
		vk.mipSamplerRepeat = oldRepeat;
		vk.mipSamplerClamp  = oldClamp;
		return;
	}

	for (i = 0; i < tr.numImages; i++)
	{
		image_t               *image = tr.images[i];
		VkDescriptorImageInfo imageDescInfo;
		VkWriteDescriptorSet  writeSet;
		VkSampler             sampler;

		if (!image || !image->mipmap || !image->vkDescriptorSet || !image->vkImageView)
		{
			continue;
		}

		sampler = VK_SamplerForImage(qtrue, image->wrapClampMode);

		Com_Memset(&imageDescInfo, 0, sizeof(imageDescInfo));
		imageDescInfo.sampler     = sampler;
		imageDescInfo.imageView   = (VkImageView)image->vkImageView;
		imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		Com_Memset(&writeSet, 0, sizeof(writeSet));
		writeSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.dstSet          = (VkDescriptorSet)image->vkDescriptorSet;
		writeSet.dstBinding      = 0;
		writeSet.descriptorCount = 1;
		writeSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeSet.pImageInfo      = &imageDescInfo;

		vkUpdateDescriptorSets(vk.device, 1, &writeSet, 0, NULL);
		image->vkSampler = (void *)sampler;

		if (image->vkBindlessSlot)
		{
			VK_BindlessWrite(image->vkBindlessSlot - 1, (VkImageView)image->vkImageView, sampler);
		}
	}

	vkDestroySampler(vk.device, oldRepeat, NULL);
	vkDestroySampler(vk.device, oldClamp, NULL);
}

/**
 * @brief Values for the final gamma pass (set every frame by
 * R_ScreenGamma(); renderer1's u_gamma / u_overBrightBits uniforms).
 */
void VK_SetGammaParams(float gamma, float overbrightScale)
{
	vk.gammaValue      = (gamma > 0.0f) ? gamma : 1.0f;
	vk.overbrightScale = overbrightScale;
}

/**
 * @brief renderer1's RE_UploadCinematic texture update for a
 * cinematic scratch image: sub-image update in place when the size is
 * unchanged, otherwise a new single-level, clamped, linear image at the
 * raw cinematic size (the old one is freed after the current frame).
 */
qboolean VK_UploadScratchImage(struct image_s *image, const byte *data, int cols, int rows)
{
	VkDeviceSize         size = (VkDeviceSize)cols * (VkDeviceSize)rows * 4;
	VkBuffer             stagingBuffer;
	VkDeviceMemory       stagingMemory;
	void                 *mapped;
	VkCommandBuffer      cmd;
	VkImageMemoryBarrier barrier;
	VkBufferImageCopy    region;

	if (!vk.initialized || !image || !data || cols <= 0 || rows <= 0)
	{
		return qfalse;
	}

	if (!image->vkImage || image->uploadWidth != cols || image->uploadHeight != rows)
	{
		if (image->vkImage)
		{
			VK_QueueImageResourcesForFree(image);
		}

		image->mipmap        = qfalse;
		image->wrapClampMode = GL_CLAMP_TO_EDGE;
		image->uploadWidth   = cols;
		image->uploadHeight  = rows;

		return VK_CreateImageTexture(image, data, cols, rows, 1, qfalse, GL_CLAMP_TO_EDGE);
	}

	if (!VK_CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                     &stagingBuffer, &stagingMemory))
	{
		return qfalse;
	}

	vkMapMemory(vk.device, stagingMemory, 0, size, 0, &mapped);
	Com_Memcpy(mapped, data, (size_t)size);
	vkUnmapMemory(vk.device, stagingMemory);

	cmd = VK_BeginOneShotCommands();

	Com_Memset(&barrier, 0, sizeof(barrier));
	barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout                   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
	barrier.image                       = (VkImage)image->vkImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask               = VK_ACCESS_SHADER_READ_BIT;
	barrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	Com_Memset(&region, 0, sizeof(region));
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width           = (uint32_t)cols;
	region.imageExtent.height          = (uint32_t)rows;
	region.imageExtent.depth           = 1;
	vkCmdCopyBufferToImage(cmd, stagingBuffer, (VkImage)image->vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, NULL, 0, NULL, 1, &barrier);

	VK_EndOneShotCommands(cmd);

	vkDestroyBuffer(vk.device, stagingBuffer, NULL);
	vkFreeMemory(vk.device, stagingMemory, NULL);

	return qtrue;
}

/**
 * @brief Active MSAA sample count (1 = off).
 */
int VK_GetSampleCount(void)
{
	return (int)vk.sampleCount;
}

/**
 * @brief Draws immediate-mode style geometry (positions,
 * per-vertex colors and texcoords) as a triangle or line list with the
 * given GL state bits, two-sided, using the current 3D matrices.
 */
void VK_DrawImmediate3D(const float (*xyz)[4], const byte (*colors)[4], const float (*st)[2], int numVertexes,
                        const unsigned int *indexes, int numIndexes, qboolean lines, struct image_s *image, unsigned long stateBits)
{
	VkDeviceSize    vertexBytes, indexBytes;
	vk3DVertex_t    *dstVerts;
	float           pushConstants[48];
	VkDescriptorSet descriptorSet;
	VkPipeline      pipeline;
	int             i;

	if (!vk.initialized || numVertexes <= 0 || numIndexes <= 0)
	{
		return;
	}
	if (!image || !image->vkDescriptorSet)
	{
		image = tr.whiteImage;
	}
	if (!image || !image->vkDescriptorSet)
	{
		return;
	}

	pipeline = VK_GetScenePipeline(stateBits & ~GLS_POLYMODE_LINE, CT_TWO_SIDED, qfalse, qfalse,
	                               lines ? VK_SPECIAL_LINES : VK_SPECIAL_NONE);
	if (pipeline == VK_NULL_HANDLE)
	{
		return;
	}

	// draws address the arena with vertexOffset (in vertices): keep the
	// byte offset a multiple of this vertex type's stride
	vk.sceneVertexArenaOffset = VK_ALIGN_UP(vk.sceneVertexArenaOffset, sizeof(vk3DVertex_t));
	vertexBytes               = (VkDeviceSize)numVertexes * sizeof(vk3DVertex_t);
	indexBytes                = (VkDeviceSize)numIndexes * sizeof(uint32_t);
	if (vk.sceneVertexArenaOffset + vertexBytes > vk.sceneVertexArenaSize ||
	    vk.sceneIndexArenaOffset + indexBytes > vk.sceneIndexArenaSize)
	{
		return;
	}

	dstVerts = (vk3DVertex_t *)((byte *)vk.sceneVertexArenaMapped + vk.sceneVertexArenaOffset);
	for (i = 0; i < numVertexes; i++)
	{
		dstVerts[i].pos[0]   = xyz[i][0];
		dstVerts[i].pos[1]   = xyz[i][1];
		dstVerts[i].pos[2]   = xyz[i][2];
		dstVerts[i].uv[0]    = st ? st[i][0] : 0.0f;
		dstVerts[i].uv[1]    = st ? st[i][1] : 0.0f;
		dstVerts[i].color[0] = colors[i][0];
		dstVerts[i].color[1] = colors[i][1];
		dstVerts[i].color[2] = colors[i][2];
		dstVerts[i].color[3] = colors[i][3];
	}
	Com_Memcpy((byte *)vk.sceneIndexArenaMapped + vk.sceneIndexArenaOffset, indexes, (size_t)indexBytes);

	VK_BindPipelineCached(pipeline);

	Com_Memcpy(pushConstants, vk.mvpMatrix, sizeof(vk.mvpMatrix));
	Com_Memcpy(pushConstants + 16, vk.modelMatrix, sizeof(vk.modelMatrix));
	pushConstants[32] = VK_AlphaTestMode(stateBits);
	pushConstants[33] = 0.0f;
	pushConstants[34] = vk.fogExp ? 1.0f : 0.0f;
	pushConstants[35] = vk.fogDensity;
	pushConstants[36] = vk.fogColor[0];
	pushConstants[37] = vk.fogColor[1];
	pushConstants[38] = vk.fogColor[2];
	pushConstants[39] = vk.fogColor[3];
	pushConstants[40] = vk.fogEnabled ? 1.0f : 0.0f;
	pushConstants[41] = vk.fogStart;
	pushConstants[42] = vk.fogEnd;
	pushConstants[43] = vk.clipPlaneEnabled ? 1.0f : 0.0f;
	pushConstants[44] = vk.clipPlane[0];
	pushConstants[45] = vk.clipPlane[1];
	pushConstants[46] = vk.clipPlane[2];
	pushConstants[47] = vk.clipPlane[3];
	VK_PushConstantsCached(vk.scenePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushConstants, sizeof(pushConstants));

	descriptorSet = (VkDescriptorSet)image->vkDescriptorSet;
	VK_BindDescriptorSetsCached(vk.scenePipelineLayout, 1, &descriptorSet);

	VK_BindArenaBuffers(vk.sceneVertexArenaBuffer, vk.sceneIndexArenaBuffer);
	vkCmdDrawIndexed(vk.commandBuffer, (uint32_t)numIndexes, 1, (uint32_t)(vk.sceneIndexArenaOffset / sizeof(uint32_t)), (int32_t)(vk.sceneVertexArenaOffset / sizeof(vk3DVertex_t)), 0);

	vk.sceneVertexArenaOffset += vertexBytes;
	vk.sceneIndexArenaOffset  += indexBytes;
}
