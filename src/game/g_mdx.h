/*

    Copyright (C) 2003-2005 Christopher Lais (aka "Zinx Verituse")
    and is covered by the following license:

    ***
    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. Modified source for this software, as used in any binaries you have
    distributed, must be provided on request, free of charge and/or penalty.

    4. This notice may not be removed or altered from any source distribution.
    ***

*/

#ifndef INCLUDE_G_MDX_H
#define INCLUDE_G_MDX_H

//TODO: enable this later on when fixed (&& figured out how the fuck it works)
/*
#ifdef ETLEGACY_DEBUG
#define BONE_HITTESTS 1
#endif
*/

#define REALHEAD_BONEHITS   128

#define REALHEAD_DEBUG_HEAD 256
#define REALHEAD_DEBUG_LEGS 512
#define REALHEAD_DEBUG_BODY 1024

/* cut down refEntity_t w/ only stuff needed for player bone calculation
   Used only by game code - not engine
   This struct has been moved to g_local.h

typedef struct {
    qhandle_t	hModel;				// opaque type outside refresh

    vec3_t		headAxis[3];

    // most recent data
    vec3_t		axis[3];		// rotation vectors
    vec3_t		torsoAxis[3];		// rotation vectors for torso section of skeletal animation
//	qboolean	nonNormalizedAxes;	// axis are not normalized, i.e. they have scale
    float		origin[3];
    int			frame;
    qhandle_t	frameModel;
    int			torsoFrame;			// skeletal torso can have frame independant of legs frame
    qhandle_t	torsoFrameModel;

    // previous data for frame interpolation
    float		oldorigin[3];
    int			oldframe;
    qhandle_t	oldframeModel;
    int			oldTorsoFrame;
    qhandle_t	oldTorsoFrameModel;
    float		backlerp;			// 0.0 = current, 1.0 = old
    float		torsoBacklerp;
} grefEntity_t;
*/

/* ******************* MDM/MDX file format, etc */
/**
 * @struct mdm_hdr
 * @brief The mdm_hdr struct
 * @note From http://games.theteamkillers.net/rtcw/mdx/ (linky is dead)
 */
struct mdm_hdr
{
	char ident[4];              ///< "MDMW"
	byte version[4];            ///< uint32
	char filename[MAX_QPATH];
	byte lod_bias[4];           ///< vec_t
	byte lod_scale[4];          ///< vec_t
	byte surface_count[4];      ///< uint32
	byte surface_offset[4];     ///< uint32
	byte tag_count[4];          ///< uint32
	byte tag_offset[4];         ///< uint32
	byte eof_offset[4];         ///< uint32
};

/**
 * @struct mdm_tag
 * @brief
 */
struct mdm_tag
{
	char name[64];
	byte axis[3][3][4];         ///< vec_t[3][3]
	byte attach_bone[4];        ///< uint32
	byte offset[3][4];          ///< vec_t[3]
	byte bone_count[4];         ///< uint32
	byte bone_offset[4];        ///< uint32
	byte tag_size[4];           ///< uint32
	// bone indexes (uint32) follow
};

/**
 * @struct mdx_hdr
 * @brief
 */
struct mdx_hdr
{
	char ident[4];              ///< "MDXW"
	byte version[4];            ///< uint32
	char filename[MAX_QPATH];
	byte frame_count[4];        ///< uint32
	byte bone_count[4];         ///< uint32
	byte frame_offset[4];       ///< uint32
	byte bone_offset[4];        ///< uint32
	byte torso_parent[4];       ///< uint32
	byte eof_offset[4];         ///< uint32
};

/**
 * @struct mdx_frame_bone
 * @brief
 */
struct mdx_frame_bone
{
	byte angles[3][2];          ///< int16[3]
	byte unused[2];             ///< int16
	byte offset_angles[2][2];   ///< int16[2]
};

/**
 * @struct mdx_frame
 * @brief
 */
struct mdx_frame
{
	byte mins[3][4];            ///< vec_t[3]
	byte maxs[3][4];            ///< vec_t[3]
	byte origin[3][4];          ///< vec_t[3]
	byte radius[4];             ///< vec_t
	byte parent_offset[3][4];   ///< vec_t[3]
	// mdx_frame_bones follow
};

/**
 * @struct mdx_bone
 * @brief
 */
struct mdx_bone
{
	char name[64];
	byte parent_index[4];       ///< int32
	byte torso_weight[4];       ///< vec_t
	byte parent_dist[4];        ///< vec_t
	byte is_tag[4];             ///< uint32
};

/**
 * @struct mdx
 * @brief
 */
struct mdx
{
	struct mdx_hdr *hdr;
	void *frame;            ///< struct mdx_frame; struct mdx_frame_bone*bone_count; ...
	struct mdx_bone *bone;
	int frames, bones;
};


//////////////////////

/**
 * @struct bone
 * @brief
 */
struct bone
{
	char name[64];
	int parent_index;
	vec_t parent_dist;
	vec_t torso_weight;
};

/**
 * @struct frame_bone
 * @brief
 */
struct frame_bone
{
	short angles[3];            ///< Orientation angle
	short offset_angles[2];     ///< Offset angle
	float anglesF[3];           ///< floating point values instead of short integers
	float offset_anglesF[2];    ///< floating point values instead of short integers
};

/**
 * @struct frame
 * @brief
 */
struct frame
{
	vec_t radius;
	vec3_t parent_offset;
	struct frame_bone *bones;
};

/**
 * @struct hit_area
 * @brief
 */
struct hit_area
{
	int hit_type;
	animScriptImpactPoint_t impactpoint;

	int tag[2];             ///< internal (cached) tag numbers
	vec3_t scale[2];
	qboolean ishead[2];
	qboolean isbox;

	vec3_t axis[3];         ///< additional axis rotation before scale
};

/**
 * @struct tag
 * @brief
 */
struct tag
{
	char name[64];
	vec3_t axis[3];
	vec3_t offset;
	int attach_bone;
};

////////////////////////////////////////////

#ifdef BONE_HITTESTS
#define TAG_INTERNAL        (1 << 30)
#define TAG_INTERNAL_MASK   (~TAG_INTERNAL)

#define INTERNTAG_TAG       (1 << 29) // based off tag, not bone
#define INTERNTAG_TAG_MASK  (~INTERNTAG_TAG)
#endif

/**
 * @struct interntag_s
 * @brief
 */
typedef struct interntag_s
{
	struct tag tag;
	qboolean merged;    ///< merge with cachetag
	float weight;       ///< weight for merge (1.0 = only first)
	qboolean ishead;    ///< use head angles (for offset)
} interntag_t;

/**
 * @struct mdm_s
 * @typedef mdm_t
 * @brief
 */
typedef struct mdm_s
{
	char path[MAX_QPATH];

	int tag_count;
	struct tag *tags;

	// quick lookup
	int tag_head, tag_footleft, tag_footright;
#ifdef BONE_HITTESTS
	int *cachetags; // cachetag_count entries
#endif // BONE_HITTESTS
} mdm_t;

/**
 * @struct mdx_s
 * @typedef mdx_t
 * @brief
 */
typedef struct mdx_s
{
	char path[MAX_QPATH];

	int bone_count;
	struct bone *bones;

	int frame_count;
	struct frame *frames;

	int torso_parent;
} mdx_t;

/**
 * @struct hit_s
 * @typedef hit_t
 * @brief
 */
typedef struct hit_s
{
	const animModelInfo_t *animModelInfo;

	int hit_count;
	struct hit_area *hits;
} hit_t;

extern void mdx_cleanup(void);

extern qhandle_t trap_R_RegisterModel(const char *filename);

extern int trap_R_LerpTagNumber(orientation_t *tag, /*const*/ grefEntity_t *refent, int tagNum);

extern int trap_R_LookupTag(/*const*/ grefEntity_t *refent, const char *tagName);
extern int trap_R_LerpTag(orientation_t *tag, /*const*/ grefEntity_t *refent, const char *tagName, int startIndex);

extern void mdx_head_position(/*const*/ gentity_t *ent, /*const*/ grefEntity_t *refent, vec3_t org);
extern void mdx_legs_position(/*const*/ gentity_t *ent, /*const*/ grefEntity_t *refent, vec3_t org);


#define RHTAG_CHEST 1
#define RHTAG_WEAPON 2
#define RHTAG_WEAPON2 3

extern void mdx_tag_position(gentity_t *ent, grefEntity_t *refent, vec3_t org, const char *tagName, float up_offset, float forward_offset);

extern void mdx_PlayerAngles(gentity_t *ent, vec3_t legsAngles, vec3_t torsoAngles, vec3_t headAngles, qboolean doswing);
extern void mdx_PlayerAnimation(gentity_t *ent);
extern void mdx_gentity_to_grefEntity(gentity_t *ent, grefEntity_t *refent, int lerpTime);

void mdx_LoadHitsFile(char *animationGroup, animModelInfo_t *animModelInfo);

typedef enum
{
    MRP_NECK,
    MRP_ELBOW_LEFT,
    MRP_ELBOW_RIGHT,
    MRP_HAND_LEFT,
    MRP_HAND_RIGHT,
    MRP_BACK,
    MRP_CHEST,
    MRP_PELVIS,
    MRP_KNEE_LEFT,
    MRP_KNEE_RIGHT,
    MRP_ANKLE_LEFT,
    MRP_ANKLE_RIGHT,
    MRP_MAX,
} mdx_advanced_position_t;

//extern void mdx_advanced_positions(gentity_t &ent, grefEntity_t &re, vec3_t*, orientation_t*);
//extern void mdx_weapon_positions(gentity_t &ent, grefEntity_t &re, vec3_t*, orientation_t*);

#ifdef BONE_HITTESTS
enum
{
	MDX_NONE = 0,
	MDX_GUN,
	MDX_HEAD,
	MDX_TORSO,
	MDX_ARM_L,
	MDX_ARM_R,
	MDX_LEG_L,
	MDX_LEG_R,
	MDX_HIT_TYPE_MAX
};

extern qhandle_t mdx_RegisterHits(animModelInfo_t *animModelInfo, const char *filename);
extern qboolean mdx_hit_test(const vec3_t start, const vec3_t end, /*const*/ gentity_t *ent, /*const*/ grefEntity_t *refent, int *hit_type, vec_t *fraction, animScriptImpactPoint_t *impactpoint);
#endif

#endif // INCLUDE_G_MDX_H
