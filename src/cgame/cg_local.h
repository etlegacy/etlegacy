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
 * @file cg_local.h
 *
 * The entire cgame module is unloaded and reloaded on each level change, so
 * there is NO persistant data between levels on the client side.
 * If you absolutely need something stored, it can either be kept by the server
 * in the server stored userinfos, or stashed in a cvar.
 */

#ifndef INCLUDE_CG_LOCAL_H
#define INCLUDE_CG_LOCAL_H

#include "../qcommon/q_shared.h"
#include "../qcommon/q_unicode.h"
#include "../renderercommon/tr_types.h"
#include "../game/bg_public.h"
#include "cg_public.h"
#include "../ui/ui_shared.h"


#define STATS_FADE_TIME     200
#define FADE_TIME           200
#define DAMAGE_DEFLECT_TIME 100
#define DAMAGE_RETURN_TIME  400
#define LAND_DEFLECT_TIME   150
#define LAND_RETURN_TIME    300
#define STEP_TIME           200
#define DUCK_TIME           100
#define PAIN_TWITCH_TIME    200
#define ZOOM_TIME           150.0f
#define MUZZLE_FLASH_TIME   30
#define SINK_TIME           3000        ///< time for fragments to sink into ground before going away

#define PRONE_TIME          500

#define MAX_STEP_CHANGE     32

#define MAX_VERTS_ON_POLY   10
#define MAX_MARK_POLYS      256     ///< was 1024

#define STAT_MINUS          10      ///< num frame for '-' stats digit

#define NOTIFY_WIDTH        80
#define NOTIFY_HEIGHT       5

#define TEAMCHAT_WIDTH      70
#define TEAMCHAT_HEIGHT     8

#define NUM_CROSSHAIRS      16

// trails
#define STYPE_STRETCH   0
#define STYPE_REPEAT    1

#define TJFL_FADEIN     (1 << 0)
#define TJFL_CROSSOVER  (1 << 1)
#define TJFL_NOCULL     (1 << 2)
#define TJFL_FIXDISTORT (1 << 3)
#define TJFL_SPARKHEADFLARE (1 << 4)
#define TJFL_NOPOLYMERGE    (1 << 5)

// Autoaction values
#define AA_DEMORECORD   0x01
#define AA_SCREENSHOT   0x02
#define AA_STATSDUMP    0x04

// Cursor
#define CURSOR_OFFSETX  13
#define CURSOR_OFFSETY  12

// Demo controls
#define DEMO_THIRDPERSONUPDATE  0
#define DEMO_RANGEDELTA         6
#define DEMO_ANGLEDELTA         4

#ifdef FEATURE_MULTIVIEW
// MV overlay
#define MVINFO_RIGHT        Ccg_WideX(SCREEN_WIDTH) - 6
#define MVINFO_TOP          8
#endif

#define MAX_WINDOW_COUNT        10
#define MAX_WINDOW_LINES        64

#define MAX_STRINGS             80
#define MAX_STRING_POOL_LENGTH  128

#define WINDOW_FONTWIDTH    8       ///< For non-true-type: width to scale from
#define WINDOW_FONTHEIGHT   8       ///< For non-true-type: height to scale from

#define WID_NONE            0x00    ///< General window
#define WID_STATS           0x01    ///< Stats (reusable due to scroll effect)
#define WID_TOPSHOTS        0x02    ///< Top/Bottom-shots
#define WID_MOTD            0x04    ///< MOTD
//#define WID_DEMOHELP      0x08    ///< Demo key control info
//#define WID_SPECHELP      0x10    ///< MV spectator key control info

#define WFX_TEXTSIZING      0x01    ///< Size the window based on text/font setting
#define WFX_FLASH           0x02    ///< Alternate between bg and b2 every half second
#define WFX_TRUETYPE        0x04    ///< Use truetype fonts for text
#define WFX_MULTIVIEW       0x08    ///< Multiview window
// These need to be last
#define WFX_FADEIN          0x10    ///< Fade the window in (and back out when closing)
#define WFX_SCROLLUP        0x20    ///< Scroll window up from the bottom (and back down when closing)
#define WFX_SCROLLDOWN      0x40    ///< Scroll window down from the top (and back up when closing)
#define WFX_SCROLLLEFT      0x80    ///< Scroll window in from the left (and back right when closing)
#define WFX_SCROLLRIGHT     0x100   ///< Scroll window in from the right (and back left when closing)

#define WSTATE_COMPLETE     0x00    ///< Window is up with startup effects complete
#define WSTATE_START        0x01    ///< Window is "initializing" w/effects
#define WSTATE_SHUTDOWN     0x02    ///< Window is shutting down with effects
#define WSTATE_OFF          0x04    ///< Window is completely shutdown

#ifdef FEATURE_MULTIVIEW
#define MV_PID              0x00FF  ///< Bits available for player IDs for MultiView windows
#define MV_SELECTED         0x0100  ///< MultiView selected window flag is the 9th bit
#endif

#define ISVALIDCLIENTNUM(clientNum) ((clientNum) >= 0 && (clientNum) < MAX_CLIENTS)

/**
 * @struct specLabel_s
 * @typedef specLabel_t
 * @brief
 */
typedef struct specLabel_s
{
	float x;
	float y;
	float scale;
	const char *text;
	vec3_t origin;
	int lastVisibleTime;
	int lastInvisibleTime;
	qboolean visible;
	float alpha;
} specLabel_t;

/**
* @struct specBar_s
* @typedef specBar_t
* @brief
*/
typedef struct specBar_s
{
	float x;
	float y;
	float w;
	float h;
	float fraction;
	vec4_t colorStart;
	vec4_t colorEnd;
	vec4_t colorBack;
	vec3_t origin;
	int lastVisibleTime;
	int lastInvisibleTime;
	qboolean visible;
	float alpha;
} specBar_t;

/**
 * @struct cg_window_s
 * @brief
 */
typedef struct
{
	vec4_t colorBorder;                 ///< Window border color
	vec4_t colorBackground;             ///< Window fill color
	vec4_t colorBackground2;            ///< Window fill color2 (for flashing)
	int curX;                           ///< Scrolling X position
	int curY;                           ///< Scrolling Y position
	int effects;                        ///< Window effects
	float flashMidpoint;                ///< Flashing transition point (in ms)
	int flashPeriod;                    ///< Background flashing period (in ms)
	int fontHeight;                     ///< For non-truetype font drawing
	float fontScaleX;                   ///< Font scale factor
	float fontScaleY;                   ///< Font scale factor
	int fontWidth;                      ///< For non-truetype font drawing
	float h;                            ///< Height
	int id;                             ///< Window ID for special handling (i.e. stats, motd, etc.)
	qboolean inuse;                     ///< Activity flag
	int lineCount;                      ///< Number of lines to display
	int lineHeight[MAX_WINDOW_LINES];   ///< Height property for each line
	char *lineText[MAX_WINDOW_LINES];   ///< Text info
#ifdef FEATURE_MULTIVIEW
	float m_x;                          ///< Mouse X position
	float m_y;                          ///< Mouse Y position
	int mvInfo;                         ///< lower 8 = player id, 9 = is_selected
#endif
	int targetTime;                     ///< Time to complete any defined effect
	int state;                          ///< Current state of the window
	int time;                           ///< Current window time
	float w;                            ///< Width
	float x;                            ///< Target x-coordinate
	///< negative values will align the window from the right minus the (window width + offset(x))
	float y;                            ///< Target y-coordinate
	///< negative values will align the window from the bottom minus the (window height + offset(y))
} cg_window_t;

/**
 * @struct cg_string_s
 * @brief
 */
typedef struct
{
	qboolean fActive;
	char str[MAX_STRING_POOL_LENGTH];
} cg_string_t;

/**
 * @struct cg_windowHandler_s
 * @brief
 */
typedef struct
{
	int activeWindows[MAX_WINDOW_COUNT];    ///< List of active windows
	int numActiveWindows;                   ///< Number of active windows in use
	cg_window_t window[MAX_WINDOW_COUNT];   ///< Static allocation of all windows
} cg_windowHandler_t;

#ifdef FEATURE_MULTIVIEW
/**
 * @struct cg_mvinfo_s
 * @brief
 */
typedef struct
{
	int pID;            ///< Player ID
	int classID;        ///< Player's current class
	int width;          ///< Width of text box
	char info[8];       ///< On-screen info (w/color coding)
	qboolean fActive;   ///< Overlay element is active
	cg_window_t *w;     ///< Window handle (may be NULL)
} cg_mvinfo_t;
#endif

//=================================================

/**
 * @struct lerpFrame_s
 * @brief
 */
typedef struct
{
	int oldFrame;
	int oldFrameTime;               ///< time when ->oldFrame was exactly on
	qhandle_t oldFrameModel;

	int frame;
	int frameTime;                  ///< time when ->frame will be exactly on
	qhandle_t frameModel;

	float backlerp;

	float yawAngle;
	qboolean yawing;
	float pitchAngle;
	qboolean pitching;

	int animationNumber;            ///< may include ANIM_TOGGLEBIT
	int oldAnimationNumber;         ///< may include ANIM_TOGGLEBIT
	animation_t *animation;
	int animationTime;              ///< time when the first frame of the animation will be exact

	// variable speed anims
	vec3_t oldFramePos;
	float animSpeedScale;
	int oldFrameSnapshotTime;
	//headAnimation_t *headAnim;

} lerpFrame_t;

/**
 * @struct playerEntity_s
 * @brief Player entities need to track more information
 * than any other type of entity.
 *
 * Note that not every player entity is a client entity,
 * because corpses after respawn are outside the normal
 * client numbering range.
 *
 * When changing animation, set animationTime to frameTime + lerping time.
 * The current lerp will finish out, then it will lerp to the new animation.
 */
typedef struct
{
	lerpFrame_t legs;
	lerpFrame_t torso;
	lerpFrame_t head;
	lerpFrame_t weap;           ////< autonomous weapon animations
	lerpFrame_t hudhead;

	int painTime;
	int painDuration;
	int painDirection;          ///< flip from 0 to 1
	int painAnimTorso;
	int painAnimLegs;
	int lightningFiring;

	// so we can do fast tag grabbing
	refEntity_t bodyRefEnt, headRefEnt, gunRefEnt, handRefEnt;
	int gunRefEntFrame;

	float animSpeed;            ///< for manual adjustment

	int lastFiredWeaponTime;
	int weaponFireTime;

	// visible leaning from NQ/FalkonET
	int leanTime;
	float leanDirection;
	int leanDir;

} playerEntity_t;

//=================================================

/**
 * @struct tag_s
 * @typedef tag_t
 * @brief
 */
typedef struct tag_s
{
	vec3_t origin;
	vec3_t axis[3];
} tag_t;

/**
 * @struct centity_s
 * @typedef centity_t
 * @brief centity_t have a direct corespondence with gentity_t in the game, but
 * only the entityState_t is directly communicated to the cgame
 */
typedef struct centity_s
{
	entityState_t currentState;     ///< from cg.frame
	entityState_t nextState;        ///< from cg.nextFrame, if available
	qboolean interpolate;           ///< true if next is valid to interpolate to
	qboolean currentValid;          ///< true if cg.frame holds this entity

	int muzzleFlashTime;            ///< move to playerEntity?
	int overheatTime;
	int previousEvent;
	int previousEventSequence;
	int teleportFlag;

	int trailTime;                  ///< so missile trails can handle dropped initial packets
	int miscTime;
	int soundTime;                  ///< so looping sounds can start when triggered

	playerEntity_t pe;

	vec3_t rawOrigin;
	vec3_t rawAngles;

	// exact interpolated position of entity on this frame
	vec3_t lerpOrigin;
	vec3_t lerpAngles;

	vec3_t lastLerpAngles;          ///< for remembering the last position when a state changes
	vec3_t lastLerpOrigin;          ///< Added for linked trains player adjust prediction

	// trail effects
	int headJuncIndex;
	int headJuncIndex2;
	int lastTrailTime;

	vec3_t fireRiseDir;             ///< if standing still this will be up, otherwise it'll point away from movement dir
	int lastFuseSparkTime;

	// client side dlights
	int dl_frame;
	int dl_oldframe;
	float dl_backlerp;
	int dl_time;
	char dl_stylestring[64];
	int dl_sound;
	int dl_atten;

	lerpFrame_t lerpFrame;
	vec3_t highlightOrigin;             ///< center of the geometry.  for things like corona placement on treasure
	qboolean usehighlightOrigin;

	refEntity_t refEnt;
	int processedFrame;                 ///< frame we were last added to the scene

	int voiceChatSprite;
	int voiceChatSpriteTime;

	// item highlighting
	int highlightTime;
	qboolean highlighted;

	// spline stuff
	vec3_t origin2;
	splinePath_t *backspline;
	float backdelta;
	qboolean back;
	qboolean moving;

	int tankframe;
	int tankparent;
	tag_t mountedMG42Base;
	tag_t mountedMG42Nest;
	tag_t mountedMG42;
	tag_t mountedMG42Player;
	tag_t mountedMG42Flash;

	qboolean akimboFire;

	// tagconnect cleanup..
	int tagParent;
	char tagName[MAX_QPATH];
} centity_t;

//======================================================================

/**
 * @struct markPoly_s
 * @typedef markPoly_t
 * @brief
 */
typedef struct markPoly_s
{
	struct markPoly_s *prevMark, *nextMark;
	int time;
	qhandle_t markShader;
	qboolean alphaFade;          ///< fade alpha instead of rgb
	float color[4];
	poly_t poly;
	polyVert_t verts[MAX_VERTS_ON_POLY];

	int duration;
} markPoly_t;

/**
 * @enum leType_t
 * @brief
 */
typedef enum
{
	LE_MARK = 0,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FRAGMENT,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_CONST_RGB,
	LE_SCALE_FADE,
	LE_SPARK,
	LE_DEBRIS,
	LE_BLOOD,
	LE_FUSE_SPARK,
	LE_MOVING_TRACER,
	LE_EMITTER
} leType_t;

/**
 * @enum leFlag_t
 * @brief
 */
typedef enum
{
	LEF_PUFF_DONT_SCALE = 0x0001    ///< do not scale size over time
	, LEF_TUMBLE        = 0x0002    ///< tumble over time, used for ejecting shells
	, LEF_NOFADEALPHA   = 0x0004    ///< sparks
	, LEF_SMOKING       = 0x0008    ///< smoking
	, LEF_TUMBLE_SLOW   = 0x0010    ///< slow down tumble on hitting ground
} leFlag_t;

/**
 * @enum leMarkType_t
 * @brief Fragment local entities can leave marks on walls
 */
typedef enum
{
	LEMT_NONE = 0,
	LEMT_BLOOD
} leMarkType_t;

/**
 * @enum leBounceSoundType_t
 * @brief Fragment local entities can make sounds on impacts
 */
typedef enum
{
	LEBS_NONE = 0,
	LEBS_BLOOD,
	LEBS_ROCK,
	LEBS_WOOD,
	LEBS_BRASS,
	LEBS_METAL,
	LEBS_BONE,
	LEBS_SG_BRASS
} leBounceSoundType_t;

/**
 * @struct localEntity_s
 * @typedef localEntity_t
 * @brief Local entities are created as a result of events or predicted actions,
 * and live independantly from all server transmitted entities
 */
typedef struct localEntity_s
{
	struct localEntity_s *prev, *next;
	leType_t leType;
	int leFlags;

	int startTime;
	int endTime;
	int fadeInTime;

	float lifeRate;                     ///< 1.0 / (endTime - startTime)

	trajectory_t pos;
	trajectory_t angles;

	float bounceFactor;                 ///< 0.0 = no bounce, 1.0 = perfect

	float color[4];

	float radius;

	float light;
	vec3_t lightColor;

	leMarkType_t leMarkType;            ///< mark to leave on fragment impact
	leBounceSoundType_t leBounceSoundType;

	refEntity_t refEntity;

	int lastTrailTime;
	int headJuncIndex, headJuncIndex2;
	float effectWidth;
	int effectFlags;
	struct localEntity_s *chain;        ///< Used for grouping entities (like for flamethrower junctions)
	int onFireStart, onFireEnd;
	int ownerNum;

	int breakCount;                     ///< break-up this many times before we can break no more
	float sizeScale;

	int data1;
	int data2;

} localEntity_t;

//======================================================================

/**
 * @struct score_t
 * @brief
 */
typedef struct
{
	int client;
	int score;
	int ping;
	int time;
	int powerUps;
	int team;
	int scoreflags;
	int respawnsLeft;
#ifdef FEATURE_RATING
	// skill rating
	float rating;
#endif
#ifdef FEATURE_PRESTIGE
	int prestige;
#endif
} score_t;

/**
 * @struct clientInfo_s
 * @typedef clientInfo_t
 * @brief Each client has an associated clientInfo_t
 * that contains media references necessary to present the
 * client model and other color coded effects
 * this is regenerated each time a client's configstring changes,
 * usually as a result of a userinfo (name, model, etc) change
 */
typedef struct clientInfo_s
{
	qboolean infoValid;

	int clientNum;

	char name[MAX_QPATH];
	char cleanname[MAX_QPATH];
	team_t team;

	int botSkill;                   ///< OBSOLETE remove!
	int score;                      ///< updated by score servercmds
	vec3_t location;                ///< location (currentOrigin casted int values!) for team mode
	int health;                     ///< you only get this info about your teammates
	int powerups;                   ///< so can display quad/flag status
	int breathPuffTime;
	int cls;
	int latchedcls;

	int rank;

	int fireteam;
	int medals[SK_NUM_SKILLS];
	int skill[SK_NUM_SKILLS];
	int skillpoints[SK_NUM_SKILLS];      ///< filled OOB by +wstats
#ifdef FEATURE_PRESTIGE
	int deltaskillpoints[SK_NUM_SKILLS];
#endif

	int disguiseClientNum;

	int weapon;
	int secondaryweapon;
	int latchedweapon;
	int latchedsecondaryweapon;

	int refStatus;
	int shoutcaster;

	bg_character_t *character;

	/// caching fireteam pointer here, better than trying to work it out all the time
	fireteamData_t *fireteamData;

	/// for fireteams, has been selected
	qboolean selected;

	// Intermission stats
	float totalWeapAcc;
	float totalWeapHSpct;
	int kills;
	int deaths;
	int gibs;
	int selfKills;
	int teamKills;
	int teamGibs;
	int timeAxis;
	int timeAllies;
	int timePlayed;

#ifdef FEATURE_RATING
	// skill rating
	float rating;
	float deltaRating;
#endif

#ifdef FEATURE_PRESTIGE
	int prestige;
#endif

#ifdef FEATURE_MULTIVIEW
	// per client MV ps info
	int ammo;
	int ammoclip;
	int chargeTime;
	qboolean fCrewgun;
	int cursorHint;
	int grenadeTimeLeft;                ///< Actual time remaining
	int grenadeTimeStart;               ///< Time trigger base to compute TimeLeft
	int hintTime;
	int sprintTime;
	int weapHeat;
	int weaponState;
	int weaponState_last;
#endif

} clientInfo_t;

/**
 * @enum barrelType_t
 * @brief
 */
typedef enum
{
	W_PART_1 = 0,
	W_PART_2,
	W_PART_3,
	W_PART_4,
	W_PART_5,
	W_PART_6,
	W_PART_7,
	W_MAX_PARTS
} barrelType_t;

/**
 * @enum modelViewType_t
 * @brief
 */
typedef enum
{
	W_TP_MODEL = 0,     ///< Third person model
	W_FP_MODEL,         ///< First person model
	W_PU_MODEL,         ///< Pickup model
	W_NUM_TYPES
} modelViewType_t;

/**
 * @enum soundSurface_s
 * @typedef impactSurface_t
 * @brief index used to identify sound to play surface hit
 * wood, metal, roof, stone, glass, water, snow, flesh, carpet
 */
typedef enum soundSurface_s
{
	W_SND_SURF_DEFAULT = 0,  ///< default sound in case of no sound found for given surface
	W_SND_SURF_FAR,          ///< used sound when player is far from the origin
	W_SND_SURF_METAL,
	W_SND_SURF_WOOD,
	W_SND_SURF_GRASS,
	W_SND_SURF_GRAVEL,
	W_SND_SURF_GLASS,
	W_SND_SURF_SNOW,
	W_SND_SURF_ROOF,
	W_SND_SURF_CARPET,
	W_SND_SURF_WATER,
	W_SND_SURF_FLESH,
	W_MAX_SND_SURF

} soundSurface_t;

/**
 * @struct soundSurfaceTable_s
 * @typedef soundSurfaceTable_t
 * @brief Sound Surface Table
 */
typedef struct soundSurfaceTable_s
{
	int surfaceType;
	const char *surfaceName;

} soundSurfaceTable_t;

/**
 * @struct partModel_s
 * @typedef partModel_t
 * @brief
 */
typedef struct partModel_s
{
	char tagName[MAX_QPATH];
	qhandle_t model;
	qhandle_t skin[3];              ///< 0: neutral, 1: axis, 2: allied
} partModel_t;

/**
 * @struct weaponModel_s
 * @typedef weaponModel_t
 * @brief
 */
typedef struct weaponModel_s
{
	qhandle_t model;
	qhandle_t skin[3];              ///< 0: neutral, 1: axis, 2: allied
} weaponModel_t;

#define MAX_WEAPON_SOUNDS   5

/**
 * @struct weaponSounds_s
 * @typedef weaponSounds_t
 * @brief
 */
typedef struct weaponSounds_s
{
	int count;
	sfxHandle_t sounds[MAX_WEAPON_SOUNDS];
} weaponSounds_t;

#define MAX_IMPACT_PARTICLE 8
#define MAX_IMPACT_PARTICLE_EFFECT 2

/**
 * @struct impactParticleEffect_s
 * @typedef impactParticleEffect_t
 * @brief
 */
typedef struct impactParticleEffect_s
{
	qboolean particleEffectUsed;
	qhandle_t particleEffectShader;
	int particleEffectSpeed;
	float particleEffectSpeedRand;
	int particleEffectDuration;
	int particleEffectCount;
	float particleEffectRandScale;
	int particleEffectWidth;
	int particleEffectHeight;
	float particleEffectAlpha;

} impactParticleEffect_t;

/**
 * @struct impactExtraEffect_s
 * @typedef impactExtraEffect_t
 * @brief
 */
typedef struct impactExtraEffect_s
{
	qboolean extraEffectUsed;
	int extraEffectCount;
	float extraEffectOriginRand;
	float extraEffectVelocityRand;
	float extraEffectVelocityScaling;
	char extraEffectShaderName[16];
	int extraEffectDuration;
	float extraEffectDurationRand;
	int extraEffectSizeStart;
	float extraEffectSizeStartRand;
	int extraEffectSizeEnd;
	float extraEffectSizeEndRand;
	qboolean extraEffectLightAnim;

} impactExtraEffect_t;

/**
 * @struct impactParticle_s
 * @typedef impactParticle_t
 * @brief
 */
typedef struct impactParticle_s
{
	float particleDirectionOffset;
	float particleDirectionScaling;

	// specific for water effect
	// ripple
	int waterRippleRadius;
	int waterRippleLifeTime;

	// splash
	int waterSplashDuration;
	int waterSplashLight;
	vec3_t waterSplashLightColor;
	qboolean waterSplashIsSprite;

	// particle effect
	impactParticleEffect_t particleEffect[W_MAX_SND_SURF][MAX_IMPACT_PARTICLE_EFFECT];

	// particle explosion effect

	// main explosion (position on missile origin)
	char explosionShaderName[16];
	int explosionDuration;
	int explosionSizeStart;
	float explosionSizeStartRand;
	int explosionSizeEnd;
	float explosionSizeEndRand;
	qboolean explosionLightAnim;

	impactExtraEffect_t extraEffect[MAX_IMPACT_PARTICLE_EFFECT];

	// debris
	int debrisSpeed;
	float debrisSpeedRand;
	int debrisDuration;
	float debrisDurationRand;
	int debrisCount;
	int debrisCountExtra;
	qboolean debrisForBullet;

} impactParticle_t;


/**
 * @struct impactParticleTable_s
 * @typedef impactParticleTable_t
 * @brief
 */
typedef struct impactParticleTable_s
{
	char impactParticleName[MAX_QPATH];
	impactParticle_t impactParticle;

}impactParticleTable_t;

/**
 * @struct weaponInfo_s
 * @typedef weaponInfo_t
 * @brief each WP_* weapon enum has an associated weaponInfo_t
 * that contains media references necessary to present the weapon and its effects
 */
typedef struct weaponInfo_s
{
	qboolean registered;

	animation_t weapAnimations[MAX_WP_ANIMATIONS];

	qhandle_t handsModel;               ///< the hands don't actually draw, they just position the weapon

	qhandle_t standModel;               ///< not drawn.  tags used for positioning weapons for pickup
	qboolean droppedAnglesHack;

	weaponModel_t weaponModel[W_NUM_TYPES];
	partModel_t partModels[W_NUM_TYPES][W_MAX_PARTS];
	qhandle_t flashModel[W_NUM_TYPES];
	qhandle_t modModels[6];             ///< like the scope for the rifles

	vec3_t flashDlightColor;
	weaponSounds_t flashSound;          ///< fast firing weapons randomly choose
	weaponSounds_t flashEchoSound;      ///< distant gun firing sound
	weaponSounds_t lastShotSound;       ///< sound of the last shot can be different (mauser doesn't have bolt action on last shot for example)

	qhandle_t weaponIcon[2];            ///< [0] is weap icon, [1] is highlight icon
	int weaponIconScale;
	qhandle_t weaponSimpleIcon;
	vec2_t weaponSimpleIconScale;

	qhandle_t weaponCardIcon;
	vec2_t weaponCardScale;
	vec2_t weaponCardPointS;
	vec2_t weaponCardPointT;

	qhandle_t missileModel;
	qhandle_t missileAlliedSkin;
	qhandle_t missileAxisSkin;
	sfxHandle_t missileSound;
	weaponSounds_t missileFallSound;
	weaponSounds_t missileBouncingSound[W_MAX_SND_SURF];
	void (*missileTrailFunc)(centity_t *, const struct weaponInfo_s *wi);
	float missileDlight;
	vec3_t missileDlightColor;
	int missileRenderfx;

	void (*ejectBrassFunc)(centity_t *);
	vec3_t ejectBrassOffset;

	vec3_t fireRecoil;                  ///< kick angle
	vec3_t adjustLean;

	sfxHandle_t readySound;             ///< an ambient sound the weapon makes when it's /not/ firing
	sfxHandle_t firingSound;
	sfxHandle_t overheatSound;
	sfxHandle_t reloadSound;
	sfxHandle_t reloadFastSound;

	sfxHandle_t spinupSound;        ///< sound started when fire button goes down, and stepped on when the first fire event happens
	sfxHandle_t spindownSound;      ///< sound called if the above is running but player doesn't follow through and fire

	sfxHandle_t switchSound;
	sfxHandle_t noAmmoSound;

	int impactDurationCoeff;
	int impactMarkMaxRange;
	int impactSoundRange;
	int impactSoundVolume;
	float impactMarkRadius;
	sfxHandle_t impactMark[W_MAX_SND_SURF];
	weaponSounds_t impactSound[W_MAX_SND_SURF];
	impactParticle_t *impactParticle;
} weaponInfo_t;

#define MAX_VIEWDAMAGE  8

/**
 * @struct viewDamage_t
 * @brief
 */
typedef struct
{
	int damageTime, damageDuration;
	float damageX, damageY, damageValue;
} viewDamage_t;

//======================================================================

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS    16

#define MAX_SPAWN_VARS          64
#define MAX_SPAWN_VARS_CHARS    2048

#define MAX_SPAWNPOINTS 32
#define MAX_SPAWNDESC   128

#define MAX_BUFFERED_SOUNDSCRIPTS 16
#define MAX_SOUNDSCRIPT_SOUNDS 16

#define MAX_FLOATING_STRINGS 128

#define MAX_FLOATING_BARS 64

/**
 * @struct soundScriptHandle_s
 * @typedef soundScriptHandle_t
 * @brief
 */
typedef struct soundScriptHandle_s
{
	char filename[MAX_QPATH];
	sfxHandle_t sfxHandle;
} soundScriptHandle_t;

/**
 * @struct soundScriptSound_s
 * @typedef soundScriptSound_t
 * @brief
 */
typedef struct soundScriptSound_s
{
	soundScriptHandle_t sounds[MAX_SOUNDSCRIPT_SOUNDS];

	int numsounds;
	int lastPlayed;

	struct soundScriptSound_s *next;
} soundScriptSound_t;

/**
 * @struct soundScript_s
 * @typedef soundScript_t
 * @brief
 */
typedef struct soundScript_s
{
	int index;
	char name[MAX_QPATH];
	int channel;
	int attenuation;
	qboolean streaming;
	qboolean looping;
	qboolean random;                    ///< TODO
	int numSounds;
	soundScriptSound_t *soundList;      ///< pointer into the global list of soundScriptSounds (defined below)

	struct soundScript_s *nextHash;     ///< next soundScript in our hashTable list position
} soundScript_t;

/**
 * @struct mapEntityData_t
 * @brief
 */
typedef struct
{
	int x, y, z;
	int yaw;
	int data;
	int type;

	vec2_t transformed;
	vec2_t automapTransformed;

	team_t team;
} mapEntityData_t;

/**
 * @enum showView_t
 * @brief
 */
typedef enum
{
	SHOW_OFF = 0,
	SHOW_SHUTDOWN,
	SHOW_ON
} showView_t;

void CG_ParseMapEntityInfo(int axis_number, int allied_number);

#define MAX_WEAP_BANKS_MP          10
#define MAX_WEAPS_IN_BANK_MP       18
#define MAX_WEAP_BANK_SWITCH_ORDER 4

#define MAX_BACKUP_STATES (CMD_BACKUP + 2)

/**
 * @struct cg_t
 * @brief
 */
typedef struct
{
	int clientFrame;                        ///< incremented each frame

	int clientNum;
	int xp;
	int xpChangeTime;

	qboolean demoPlayback;
	demoPlayInfo_t *demoinfo;
	int etLegacyClient;                     ///< is either 0 (vanilla client) or a version integer from git_version.h
	qboolean loading;                       ///< don't defer players at initial startup
	qboolean intermissionStarted;           ///< don't draw disconnect icon/message because game will end shortly

	// there are only one or two snapshot_t that are relevent at a time
	int latestSnapshotNum;                  ///< the number of snapshots the client system has received
	int latestSnapshotTime;                 ///< the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t *snap;                       ///< cg.snap->serverTime <= cg.time
	snapshot_t *nextSnap;                   ///< cg.nextSnap->serverTime > cg.time, or NULL
	snapshot_t activeSnapshots[2];

	float frameInterpolation;               ///< (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)

	qboolean thisFrameTeleport;
	qboolean nextFrameTeleport;

	int frametime;                          ///< cg.time - cg.oldTime

	int time;                               ///< this is the time value that the client
	///< is rendering at.
	int oldTime;                            ///< time at last frame, used for missile trails and prediction checking

	int physicsTime;                        ///< either cg.snap->time or cg.nextSnap->time

	int timelimitWarnings;                  ///< 5 min, 1 min, overtime

	qboolean mapRestart;                    ///< set on a map restart to set back the weapon

	qboolean renderingThirdPerson;          ///< during deaths, chasecams, etc

	// prediction state
	qboolean hyperspace;                    ///< true if prediction has hit a trigger_teleport
	playerState_t predictedPlayerState;
	centity_t predictedPlayerEntity;
	qboolean validPPS;                      ///< clear until the first call to CG_PredictPlayerState
	int predictedErrorTime;
	vec3_t predictedError;

	int eventSequence;
	int predictableEvents[MAX_PREDICTED_EVENTS];

	float stepChange;                       ///< for stair up smoothing
	int stepTime;

	float duckChange;                       ///< for duck viewheight smoothing
	int duckTime;
	qboolean wasProne;

	int weaponSetTime;                      ///< mg/mortar set time

	float landChange;                       ///< for landing hard
	int landTime;

	/// input state sent to server
	int weaponSelect;

	/// auto rotating items
	vec3_t autoAnglesSlow;
	vec3_t autoAxisSlow[3];
	vec3_t autoAngles;
	vec3_t autoAxis[3];
	vec3_t autoAnglesFast;
	vec3_t autoAxisFast[3];

	// view rendering
	refdef_t refdef;
	vec3_t refdefViewAngles;                ///< will be converted to refdef.viewaxis

	// zoom key
	qboolean zoomed;                     ///< zoomed by sniper/snooper (zoomedScope)
	qboolean zoomedBinoc;
	int zoomTime;
	float zoomSensitivity;
	float zoomval;

	/// information screen text during loading
	char infoScreenText[MAX_STRING_CHARS];

	// scoreboard
	int scoresRequestTime;
	int mapVoteListTime;                     ///< MAPVOTE - gametype 6
	int numScores;
	int selectedScore;
	int teamScores[2];
	int teamPlayers[TEAM_NUM_TEAMS];         ///< for scoreboard
	float teamPingMean[TEAM_NUM_TEAMS];
	float teamPingSd[TEAM_NUM_TEAMS];
	score_t scores[MAX_CLIENTS];
	qboolean showScores;
	qboolean scoreBoardShowing;
	int scoreFadeTime;
	char spectatorList[MAX_STRING_CHARS];    ///< list of names
	int spectatorLen;                        ///< length of list
	float spectatorWidth;                    ///< width in device units
	int spectatorTime;                       ///< next time to offset
	int spectatorPaintX;                     ///< current paint x
	int spectatorPaintX2;                    ///< current paint x
	int spectatorOffset;                     ///< current offset from start
	int spectatorPaintLen;                   ///< current offset from start

	qboolean lightstylesInited;

	// centerprinting
	int centerPrintTime;
	int centerPrintCharHeight;
	int centerPrintCharWidth;
	float centerPrintFontScale;
	int centerPrintY;
	char centerPrint[1024];
	int centerPrintLines;
	int centerPrintPriority;

	// fade in/out
	int fadeTime;
	float fadeRate;
	vec4_t fadeColor1;
	vec4_t fadeColor2;

	// game stats
	int exitStatsTime;
	int exitStatsFade;

	// kill timers for carnage reward
	int lastKillTime;

	// crosshair client-, mine-, dyna-ID
	int crosshairClientNum;
	int crosshairClientTime;
	int crosshairMine;
	int crosshairMineTime;
	int crosshairDyna;
	int crosshairDynaTime;

	qboolean crosshairNotLookingAtClient;
	int crosshairSPClientTime;
	int crosshairVerticalShift;
	qboolean crosshairClientNoShoot;
	qboolean crosshairTerrain;

	int teamFirstBlood;                         ///< 0: allies 1: axis -1: nobody
	int teamWonRounds[2];

	qboolean filtercams;

	int crosshairPowerupNum;
	int crosshairPowerupTime;

	int identifyClientRequest;

	// cursorhints
	int cursorHintIcon;
	int cursorHintTime;
	int cursorHintFade;
	int cursorHintValue;

	// attacking player
	int attackerTime;
	int voiceTime;

	// warmup countdown
	int warmup;
	int warmupCount;

	//==========================

	int weaponSelectTime;
	int weaponAnimation;
	int weaponAnimationTime;

	// blend blobs
	viewDamage_t viewDamage[MAX_VIEWDAMAGE];
	float damageTime;                           ///< last time any kind of damage was recieved
	int damageIndex;                            ///< slot that was filled in
	float damageX, damageY, damageValue;

	int grenLastTime;

	int switchbackWeapon;
	int lastFiredWeapon;
	int lastFiredWeaponTime;
	int painTime;
	int weaponFireTime;
	int nextIdleTime;
	int lastIdleTimeEnd;
	hudHeadAnimNumber_t idleAnim;
	int lastWeapSelInBank[MAX_WEAP_BANKS_MP];   ///< remember which weapon was last selected in a bank for 'weaponbank' commands

	// view movement
	float v_dmg_time;
	float v_dmg_pitch;
	float v_dmg_roll;
	float v_dmg_angle;

	vec3_t kick_angles;                         ///< weapon kicks
	vec3_t kick_origin;

	// view flames when getting burnt
	int v_fireTime, v_noFireTime;
	vec3_t v_fireRiseDir;

	// temp working variables for player view
	float bobfracsin;
	int bobcycle;
	float lastvalidBobfracsin;
	int lastvalidBobcycle;
	float xyspeed;

	// development tool
	refEntity_t testModelEntity;
	char testModelName[MAX_QPATH];
	qboolean testGun;

	// kick angles
	vec3_t kickAVel;                            ///< for damage feedback, weapon recoil, etc
	///< This is the angular velocity, to give a smooth
	///< rotational feedback, rather than sudden jerks
	vec3_t kickAngles;                          ///< for damage feedback, weapon recoil, etc
	/// NOTE: this is not transmitted through MSG.C stream
	/// since weapon kicks are client-side, and damage feedback
	/// is rare enough that we can transmit that as an event
	float recoilPitch, recoilPitchAngle;

	// Objective info display
	qboolean limboMenu;

	int oidTeam;
	int oidPrintTime;
	int oidPrintCharHeight;
	int oidPrintCharWidth;
	float oidPrintFontScale;
	int oidPrintY;
	char oidPrint[1024];
	int oidPrintLines;

	// for voice chat buffer
	int voiceChatTime;
	int voiceChatBufferIn;
	int voiceChatBufferOut;

	int newCrosshairIndex;
	qhandle_t crosshairShaderAlt[NUM_CROSSHAIRS];

	int cameraShakeTime;
	float cameraShakePhase;
	float cameraShakeScale;
	float cameraShakeLength;

	qboolean latchAutoActions;
	qboolean latchVictorySound;

	// spawn variables
	qboolean spawning;                          ///< the CG_Spawn*() functions are valid
	int numSpawnVars;
	char *spawnVars[MAX_SPAWN_VARS][2];         ///< key / value pairs
	unsigned int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	vec2_t mapcoordsMins;
	vec2_t mapcoordsMaxs;
	vec2_t mapcoordsScale;
	qboolean mapcoordsValid;

	int numMiscGameModels;
	int numCoronas;

	qboolean showCampaignBriefing;
	qboolean showGameView;
	qboolean showFireteamMenu;
	qboolean showSpawnpointsMenu;

	qboolean shoutcastMenu;

	char spawnPoints[MAX_SPAWNPOINTS][MAX_SPAWNDESC];
	vec3_t spawnCoordsUntransformed[MAX_SPAWNPOINTS];
	vec3_t spawnCoords[MAX_SPAWNPOINTS];
	team_t spawnTeams[MAX_SPAWNPOINTS];
	team_t spawnTeams_old[MAX_SPAWNPOINTS];
	int spawnTeams_changeTime[MAX_SPAWNPOINTS];
	int spawnPlayerCounts[MAX_SPAWNPOINTS];
	int spawnCount;

	cg_string_t aStringPool[MAX_STRINGS];
	int demohelpWindow;
	cg_window_t *motdWindow;
	cg_window_t *msgWstatsWindow;
	cg_window_t *msgWtopshotsWindow;
#ifdef FEATURE_MULTIVIEW
	int mv_cnt;                                     ///< Number of active MV windows
	int mvClientList;                               ///< Cached client listing of who is merged
	cg_window_t *mvCurrentActive;                   ///< Client ID of current active window (-1 = none)
	cg_window_t *mvCurrentMainview;                 ///< Client ID used in the main display (should always be set if mv_cnt > 0)
	cg_mvinfo_t mvOverlay[MAX_MVCLIENTS];           ///< Cached info for MV overlay
	int mvTeamList[TEAM_NUM_TEAMS][MAX_MVCLIENTS];
	int mvTotalClients;                             ///< Total # of clients available for MV processing
	int mvTotalTeam[TEAM_NUM_TEAMS];
#endif
	refdef_t *refdef_current;                       ///< Handling of some drawing elements for MV (not only MV!)

	qboolean showStats;
	int spechelpWindow;
	int statsRequestTime;
	cg_window_t *statsWindow;
	int topshotsRequestTime;
	cg_window_t *topshotsWindow;
	cg_window_t *windowCurrent;                     ///< Current window to update.. a bit of a hack :p
	cg_windowHandler_t winHandler;
	vec4_t xhairColor;
	vec4_t xhairColorAlt;

	// allow overriding of countdown sounds
	char fiveMinuteSound_g[MAX_QPATH];
	char fiveMinuteSound_a[MAX_QPATH];
	char twoMinuteSound_g[MAX_QPATH];
	char twoMinuteSound_a[MAX_QPATH];
	char thirtySecondSound_g[MAX_QPATH];
	char thirtySecondSound_a[MAX_QPATH];

	pmoveExt_t pmext;

	int numOIDtriggers2;
	char oidTriggerInfoAllies[MAX_OID_TRIGGERS][256];
	char oidTriggerInfoAxis[MAX_OID_TRIGGERS][256];

	int fieldopsChargeTime[2];
	int soldierChargeTime[2];
	int engineerChargeTime[2];
	int medicChargeTime[2];
	int covertopsChargeTime[2];

	char maxPlayerClasses[NUM_PLAYER_CLASSES][MAX_QPATH];

	char maxMortars[MAX_QPATH];
	char maxFlamers[MAX_QPATH];
	char maxMachineguns[MAX_QPATH];
	char maxRockets[MAX_QPATH];
	char maxRiflegrenades[MAX_QPATH];
	int maxPlayers;

	int binocZoomTime;
	int limboEndCinematicTime;
	int proneMovingTime;
	fireteamData_t fireTeams[32];

	int orderFade;
	int orderTime;

	centity_t *satchelCharge;

	playerState_t backupStates[MAX_BACKUP_STATES];
	int backupStateTop;
	int backupStateTail;
	int lastPredictedCommand;
	int lastPhysicsTime;

	qboolean skyboxEnabled;
	vec3_t skyboxViewOrg;
	vec_t skyboxViewFov;

	vec3_t tankflashorg;

	qboolean editingSpeakers;

	qboolean serverRespawning;

	// mortar hud
	vec2_t mortarFireAngles;
	int mortarImpactTime;
	vec3_t mortarImpactPos;
	qboolean mortarImpactOutOfMap;

	// artillery requests
	vec3_t artilleryRequestPos[MAX_CLIENTS];
	int artilleryRequestTime[MAX_CLIENTS];

	soundScript_t *bufferSoundScripts[MAX_BUFFERED_SOUNDSCRIPTS];
	int bufferedSoundScriptEndTime;
	int numbufferedSoundScripts;

	char objMapDescription_Axis[384];
	char objMapDescription_Allied[384];
	char objMapDescription_Neutral[384];
	char objDescription_Axis[MAX_OBJECTIVES][256];
	char objDescription_Allied[MAX_OBJECTIVES][256];

	int waterundertime;

	svCvar_t svCvars[MAX_SVCVARS];
	int svCvarCount;

	// backuping, forceCvar_t is good format, it holds name and value only
	forceCvar_t cvarBackups[MAX_SVCVARS];
	int cvarBackupsCount;

	fileHandle_t logFile;

	/// tracing bullet, predict hitboxes used on server
	qboolean bulletTrace;

	specLabel_t specOnScreenLabels[MAX_FLOATING_STRINGS];
	int specStringCount;

	specBar_t specOnScreenBar[MAX_FLOATING_BARS];
	int specBarCount;

	vec3_t airstrikePlaneScale[2];

	// objective indicator
	int flagIndicator;
	int redFlagCounter;
	int blueFlagCounter;

#if defined(FEATURE_RATING) || defined(FEATURE_PRESTIGE)
	// scoreboard
	int scoresDownTime;
	int scoreToggleTime;
#endif

#ifdef FEATURE_RATING
	// skill rating
	float rating[MAX_CLIENTS];
	float axisProb;
	float alliesProb;
#endif

#ifdef FEATURE_PRESTIGE
	int prestige[MAX_CLIENTS];
#endif

	// banner printing
	int bannerPrintTime;
	char bannerPrint[1024];
} cg_t;

#define MAX_LOCKER_DEBRIS 5
#define POSSIBLE_PIECES   6

/**
 * @struct cgMedia_t
 * @brief all of the model, shader, and sound references that are
 * loaded at gamestate time are stored in cgMedia_t
 * Other media that can be tied to clients, weapons, or items are
 * stored in the clientInfo_t, itemInfo_t, and weaponInfo_t
 */
typedef struct
{
	qhandle_t charsetShader;
	qhandle_t menucharsetShader;

	qhandle_t charsetProp;
	qhandle_t charsetPropGlow;
	qhandle_t charsetPropB;
	qhandle_t whiteShader;

	qhandle_t hudSprintBar;
	qhandle_t hudAxisHelmet;
	qhandle_t hudAlliedHelmet;
	qhandle_t hudAdrenaline;

	qhandle_t teamStatusBar;

	// gib explosions
	// gib models are stored in character->gibModels see CG_ParseGibModels()
	qhandle_t gibChest;
	qhandle_t gibIntestine;
	qhandle_t gibLeg;

	// debris
	qhandle_t debBlock[6];
	qhandle_t debRock[3];
	qhandle_t debFabric[3];
	qhandle_t debWood[6];

	qhandle_t machinegunBrassModel;
	qhandle_t panzerfaustBrassModel;
	qhandle_t smallgunBrassModel;

	qhandle_t railCoreShader;
	qhandle_t ropeShader;

	qhandle_t friendShader;

	qhandle_t spawnInvincibleShader;
	qhandle_t scoreEliminatedShader;

	qhandle_t medicReviveShader;
	qhandle_t disguisedShader;
	qhandle_t voiceChatShader;
	qhandle_t balloonShader;
	qhandle_t objectiveShader;
	qhandle_t objectiveTeamShader;
	qhandle_t objectiveDroppedShader;
	qhandle_t objectiveEnemyShader;
	qhandle_t objectiveBothTEShader;
	qhandle_t objectiveBothTDShader;
	qhandle_t objectiveBothDEShader;
	qhandle_t objectiveSimpleIcon;
	qhandle_t readyShader;

	qhandle_t destroyShader;

	qhandle_t viewBloodShader;
	qhandle_t tracerShader;
	qhandle_t crosshairShader[NUM_CROSSHAIRS];
	qhandle_t lagometerShader;
	qhandle_t backTileShader;

	qhandle_t reticleShader;
	qhandle_t reticleShaderSimple;

	qhandle_t binocShader;
	qhandle_t binocShaderSimple;

	qhandle_t fleshSmokePuffShader;                 ///< for bullet hit flesh smoke puffs

	qhandle_t smokePuffShader;
	// qhandle_t smokePuffRageProShader;
	qhandle_t shotgunSmokePuffShader;
	qhandle_t waterBubbleShader;
	qhandle_t bloodTrailShader;

	// cursor hints
	// TODO: would be nice to specify these in the menu scripts instead of permanent handles...
	qhandle_t usableHintShader;
	qhandle_t notUsableHintShader;
	qhandle_t doorHintShader;
	qhandle_t doorRotateHintShader;
	qhandle_t doorLockHintShader;
	qhandle_t mg42HintShader;
	qhandle_t breakableHintShader;
	qhandle_t healthHintShader;
	qhandle_t knifeHintShader;
	qhandle_t ladderHintShader;
	qhandle_t buttonHintShader;
	qhandle_t waterHintShader;
	qhandle_t weaponHintShader;
	qhandle_t ammoHintShader;
	qhandle_t powerupHintShader;
	qhandle_t buildHintShader;
	qhandle_t disarmHintShader;
	qhandle_t reviveHintShader;
	qhandle_t dynamiteHintShader;
	qhandle_t tankHintShader;
	qhandle_t satchelchargeHintShader;
	qhandle_t uniformHintShader;

	qhandle_t commandCentreMapShader[MAX_COMMANDMAP_LAYERS];
	qhandle_t commandCentreMapShaderTrans[MAX_COMMANDMAP_LAYERS];
	qhandle_t commandCentreAutomapShader[MAX_COMMANDMAP_LAYERS];
	qhandle_t commandCentreAutomapMaskShader;
	qhandle_t commandCentreAutomapBorderShader;
	qhandle_t commandCentreAutomapBorder2Shader;
	qhandle_t commandCentreAutomapCornerShader;
	qhandle_t commandCentreAxisMineShader;
	qhandle_t commandCentreAlliedMineShader;
	qhandle_t commandCentreSpawnShader[2];
	qhandle_t blackmask;

	qhandle_t landmineHintShader;
	qhandle_t compassConstructShader;
	//qhandle_t compassDestroyShader;
	qhandle_t buddyShader;

	qhandle_t slashShader;
	qhandle_t compassShader;
	qhandle_t compass2Shader;

	qhandle_t snowShader;
	qhandle_t oilParticle;
	qhandle_t oilSlick;

	// cannon
	qhandle_t smokePuffShaderdirty;
	qhandle_t smokePuffShaderb[5];

	// blood pool
	qhandle_t bloodPool;

	// viewscreen blood animation
	qhandle_t viewBloodAni[5];
	qhandle_t viewFlashFire[16];

	// shards
	qhandle_t shardGlass1;
	qhandle_t shardGlass2;
	qhandle_t shardWood1;
	qhandle_t shardWood2;
	qhandle_t shardMetal1;
	qhandle_t shardMetal2;
	//qhandle_t   shardCeramic1; // implement this ?
	//qhandle_t   shardCeramic2;

	// see cgs.media.debBlock
	//qhandle_t shardRubble1;
	//qhandle_t shardRubble2;
	//qhandle_t shardRubble3;

	qhandle_t shardJunk[MAX_LOCKER_DEBRIS];

	qhandle_t numberShaders[11];

	qhandle_t shadowFootShader;
	qhandle_t shadowTorsoShader;

	// wall mark shaders
	qhandle_t wakeMarkShader;
	qhandle_t wakeMarkShaderAnim;
	qhandle_t bloodMarkShaders[5];
	qhandle_t bloodDotShaders[5];
	qhandle_t burnMarkShader;

	qhandle_t flamebarrel;
	qhandle_t mg42muzzleflash;

	qhandle_t airstrikePlane[2];

	qhandle_t waterSplashModel;
	qhandle_t waterSplashShader;

	qhandle_t thirdPersonBinocModel;

	// weapon effect shaders
	//qhandle_t rocketExplosionShader;

	qhandle_t bloodCloudShader;
	qhandle_t sparkParticleShader;
	qhandle_t smokeTrailShader;

	qhandle_t flamethrowerFireStream;

	qhandle_t onFireShader, onFireShader2;

	qhandle_t sparkFlareShader;

	qhandle_t spotLightShader;
	qhandle_t spotLightBeamShader;

	qhandle_t smokeParticleShader;

	qhandle_t genericConstructionShader;
	qhandle_t shoutcastLandmineShader;

	qhandle_t alliedUniformShader;
	qhandle_t axisUniformShader;

	// sounds
	sfxHandle_t noFireUnderwater;
	sfxHandle_t selectSound;
	sfxHandle_t landHurt;

	sfxHandle_t footsteps[FOOTSTEP_TOTAL][4];
	sfxHandle_t sfx_rockexp;
	sfxHandle_t sfx_brassSound[BRASSSOUND_MAX][3][2];
	sfxHandle_t sfx_rubbleBounce[3];

	sfxHandle_t gibSound;
	sfxHandle_t landSound[FOOTSTEP_TOTAL];

	sfxHandle_t fiveMinuteSound_g, fiveMinuteSound_a;
	sfxHandle_t twoMinuteSound_g, twoMinuteSound_a;
	sfxHandle_t thirtySecondSound_g, thirtySecondSound_a;

	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;
	sfxHandle_t watrGaspSound;

	sfxHandle_t underWaterSound;

	sfxHandle_t countFight;
	sfxHandle_t countPrepare;
	sfxHandle_t goatAxis;

	// hitsounds
	sfxHandle_t headShot;
	sfxHandle_t bodyShot;
	sfxHandle_t teamShot;

	sfxHandle_t grenadePulseSound[4];

	sfxHandle_t flameSound;
	sfxHandle_t flameBlowSound;
	sfxHandle_t flameStartSound;
	sfxHandle_t flameStreamSound;

	sfxHandle_t boneBounceSound;

	qhandle_t cursor;

	sfxHandle_t uniformPickup;
	sfxHandle_t minePrimedSound;

	sfxHandle_t buildDecayedSound;

	sfxHandle_t sndLimboSelect;
	sfxHandle_t sndLimboFilter;
	//sfxHandle_t sndLimboCancel;

	sfxHandle_t sndRankUp;
	sfxHandle_t sndSkillUp;

	sfxHandle_t sndMedicCall[2];

	sfxHandle_t shoveSound;

	sfxHandle_t itemPickUpSounds[ITEM_MAX_ITEMS];

	qhandle_t ccStamps[2];
	qhandle_t ccFilterPics[9];                      ///< was 10, set to 9 (we init 0-8)
	qhandle_t ccFilterBackOn;
	qhandle_t ccFilterBackOff;

	qhandle_t ccPlayerHighlight;
	qhandle_t ccConstructIcon[2];
	qhandle_t ccCmdPost[2];
	qhandle_t ccDestructIcon[3][2];
	qhandle_t ccTankIcon;
	qhandle_t skillPics[SK_NUM_SKILLS];
#ifdef FEATURE_PRESTIGE
	qhandle_t prestigePics[3];
#endif
	qhandle_t ccMortarHit;
	qhandle_t ccMortarTarget;
	qhandle_t mortarTarget;
	qhandle_t mortarTargetArrow;

	// for commandmap
	qhandle_t medicIcon;

	qhandle_t hWeaponSnd;
	qhandle_t hWeaponEchoSnd;
	qhandle_t hWeaponHeatSnd;

	qhandle_t hWeaponSnd_2;
	qhandle_t hWeaponEchoSnd_2;
	qhandle_t hWeaponHeatSnd_2;

	qhandle_t hflakWeaponSnd;                       ///< "models/mapobjects/weapons/flak_a.md3"
	qhandle_t hMountedMG42Base;                     ///< "models/mapobjects/tanks_sd/mg42nestbase.md3"
	qhandle_t hMountedMG42Nest;                     ///< "models/mapobjects/tanks_sd/mg42nest.md3"
	qhandle_t hMountedMG42;                         ///< "models/mapobjects/tanks_sd/mg42.md3"
	qhandle_t hMountedBrowning;
	qhandle_t hMountedFPMG42;
	qhandle_t hMountedFPBrowning;

	// medals
	qhandle_t medals[SK_NUM_SKILLS];
	qhandle_t medal_back;

	// new limbo stuff
	fontHelper_t limboFont1;
	fontHelper_t limboFont1_lo;
	fontHelper_t limboFont2;
	fontHelper_t limboFont2_lo;

	// loadscreen fonts
	fontHelper_t bg_loadscreenfont1;
	fontHelper_t bg_loadscreenfont2;

	qhandle_t limboNumber_roll;
	qhandle_t limboNumber_back;
	qhandle_t limboStar_roll;
	qhandle_t limboStar_back;
	qhandle_t limboWeaponNumber_off;
	qhandle_t limboWeaponNumber_on;
	qhandle_t limboWeaponCard;
	qhandle_t limboWeaponCardSurroundH;
	qhandle_t limboWeaponCardSurroundV;
	qhandle_t limboWeaponCardSurroundC;
	qhandle_t limboWeaponCardOOS;
	qhandle_t limboLight_on;
	qhandle_t limboLight_on2;
	qhandle_t limboLight_off;

	qhandle_t limboClassButtons[NUM_PLAYER_CLASSES];

	qhandle_t limboClassButton2Back_on;
	qhandle_t limboClassButton2Back_off;
	qhandle_t limboClassButton2Wedge_on;
	qhandle_t limboClassButton2Wedge_off;
	qhandle_t limboClassButtons2[NUM_PLAYER_CLASSES];

	qhandle_t limboTeamButtonBack_on;
	qhandle_t limboTeamButtonBack_off;
	qhandle_t limboTeamButtonAllies;
	qhandle_t limboTeamButtonAxis;
	qhandle_t limboTeamButtonSpec;
	qhandle_t limboBlendThingy;
	qhandle_t limboWeaponBlendThingy;
	qhandle_t limboSkillsLW;
	qhandle_t limboSkillsBS;

	qhandle_t limboCounterBorder;
	qhandle_t limboWeaponCardArrow;
	qhandle_t limboObjectiveBack[3];
	qhandle_t limboClassBar;
	qhandle_t limboBriefingButtonOn;
	qhandle_t limboBriefingButtonOff;
	qhandle_t limboBriefingButtonStopOn;
	qhandle_t limboBriefingButtonStopOff;

	qhandle_t limboSpectator;
	qhandle_t limboShoutcaster;
	qhandle_t limboRadioBroadcast;

	qhandle_t limboTeamLocked;

	qhandle_t cursorIcon;

	qhandle_t hudPowerIcon;
	qhandle_t hudSprintIcon;
	qhandle_t hudHealthIcon;

	qhandle_t pmImages[PM_NUM_TYPES];
	qhandle_t pmImageAlliesConstruct;
	qhandle_t pmImageAxisConstruct;
	qhandle_t pmImageAlliesMine;
	qhandle_t pmImageAxisMine;
	qhandle_t pmImageAlliesFlag;
	qhandle_t pmImageAxisFlag;
	qhandle_t pmImageSpecFlag;
	qhandle_t hintKey;

	qhandle_t pmImageSlime;
	qhandle_t pmImageLava;
	qhandle_t pmImageCrush;
	qhandle_t pmImageShove;

	qhandle_t hudDamagedStates[4];

	qhandle_t browningIcon;

	qhandle_t axisFlag;
	qhandle_t alliedFlag;

	qhandle_t disconnectIcon;

	qhandle_t cm_spec_icon;
	qhandle_t cm_arrow_spec;

	qhandle_t fireteamIcon;

	qhandle_t countryFlags;         ///< GeoIP

} cgMedia_t;

/**
 * @struct arenaInfo_t
 * @brief
 */
typedef struct
{
	char lmsdescription[1024];
	char description[1024];
	char axiswintext[1024];
	char alliedwintext[1024];
	char longname[128];
	vec2_t mappos;
} arenaInfo_t;

/**
 * @struct cg_campaignInfo_t
 * @brief
 */
typedef struct
{
	char campaignDescription[2048];
	char campaignName[128];
	char mapnames[MAX_MAPS_PER_CAMPAIGN][MAX_QPATH];
	vec2_t mappos[MAX_MAPS_PER_CAMPAIGN];
	arenaInfo_t arenas[MAX_MAPS_PER_CAMPAIGN];
	int mapCount;
	int current;
	vec2_t mapTC[2];
} cg_campaignInfo_t;

#define MAX_COMMAND_INFO MAX_CLIENTS

#define MAX_STATIC_GAMEMODELS   1024    ///< some maps have > 512 game models (f.e. 'raiders' with 555)
#define MAX_GAMECORONAS 1024

/**
 * @struct cg_gamemodel_s
 * @typedef cg_gamemodel_t
 * @brief
 */
typedef struct cg_gamemodel_s
{
	qhandle_t model;
	vec3_t org;
	vec3_t axes[3];
	vec_t radius;
} cg_gamemodel_t;

/**
 * @struct cg_corona_s
 * @typedef cg_corona_t
 * @brief
 */
typedef struct cg_corona_s
{
	float scale;
	vec3_t org;
	vec3_t color;
} cg_corona_t;

/**
 * @struct cg_weaponstats_s
 * @typedef cg_weaponstats_t
 * @brief
 */
typedef struct cg_weaponstats_s
{
	int numKills;
	int numHits;
	int numShots;
} cg_weaponstats_t;

/**
 * @struct gameStats_t
 * @brief
 */
typedef struct
{
	char strWS[WS_MAX][MAX_STRING_TOKENS];
	char strExtra[6][MAX_STRING_TOKENS];
	char strRank[MAX_STRING_TOKENS];
	char strSkillz[SK_NUM_SKILLS][MAX_STRING_TOKENS];
	int cWeapons;
	int cSkills;
	qboolean fHasStats;
	int nClientID;
	int nRounds;
	int fadeTime;
	int show;
	int requestTime;
} gameStats_t;

/**
 * @struct topshotStats_t
 * @brief
 */
typedef struct
{
	char strWS[WS_MAX * 2][MAX_STRING_TOKENS];
	int cWeapons;
	int fadeTime;
	int show;
	int requestTime;
} topshotStats_t;

/**
 * @struct oidInfo_s
 * @typedef oidInfo_t
 * @brief
 */
typedef struct oidInfo_s
{
	int spawnflags;
	qhandle_t customimageallies;
	qhandle_t customimageaxis;
	int entityNum;
	int objflags;
	char name[MAX_QPATH];
	vec3_t origin;
} oidInfo_t;

// Popup filters
enum
{
	POPUP_FILTER_CONNECT  = BIT(0),
	POPUP_FILTER_TEAMJOIN = BIT(1),
	POPUP_FILTER_MISSION  = BIT(2),
	POPUP_FILTER_PICKUP   = BIT(3),
	POPUP_FILTER_DEATH    = BIT(4),
};

// Big popup filters
enum
{
	POPUP_BIG_FILTER_SKILL    = BIT(0),
	POPUP_BIG_FILTER_RANK     = BIT(1),
	POPUP_BIG_FILTER_PRESTIGE = BIT(2),
};

/// Locations
#define MAX_C_LOCATIONS 1024

// locations draw bits and cvar
/*
#define LOC_FTEAM                   1
#define LOC_VCHAT                   2
#define LOC_TCHAT                   2
#define LOC_LANDMINES               4
#define LOC_KEEPUNKNOWN             8
#define LOC_SHOWCOORDS              16
#define LOC_SHOWDISTANCE            32
#define LOC_DEBUG                   512
*/

enum
{
	LOC_FTEAM        = BIT(0),
	LOC_VCHAT        = BIT(1),
	LOC_TCHAT        = BIT(1),
	LOC_LANDMINES    = BIT(2),
	LOC_KEEPUNKNOWN  = BIT(3),
	LOC_SHOWCOORDS   = BIT(4),
	LOC_SHOWDISTANCE = BIT(5),
	LOC_DEBUG        = BIT(9),
};

enum
{
	BAR_LEFT       = BIT(0),
	BAR_CENTER     = BIT(1),
	BAR_VERT       = BIT(2),
	BAR_NOHUDALPHA = BIT(3),
	BAR_BG         = BIT(4),
	// different spacing modes for use w/ BAR_BG
	BAR_BGSPACING_X0Y5 = BIT(5),
	BAR_BGSPACING_X0Y0 = BIT(6),
	BAR_LERP_COLOR     = BIT(7),
	BAR_BORDER         = BIT(8),
	BAR_BORDER_SMALL   = BIT(9),
};

/**
 * @struct objectives_t
 * @brief
 */
typedef struct
{
	int fadeTime;
	int show;
	int requestTime;
} objectives_t;

/**
 * @struct location_s
 * @typedef location_t
 * @brief
 */
typedef struct location_s
{
	int index;
	vec3_t origin;
	char message[128];
} location_t;

/**
 * @struct clientLocation_t
 * @brief
 */
typedef struct
{
	int lastLocation;
	float lastX;
	float lastY;
	float lastZ;
} clientLocation_t;

#if defined(FEATURE_RATING) && defined(FEATURE_PRESTIGE)
#define NUM_ENDGAME_AWARDS     22   ///< total number of endgame awards
#else
#if defined(FEATURE_RATING) || defined(FEATURE_PRESTIGE)
#define NUM_ENDGAME_AWARDS     21   ///< total number of endgame awards
#else
#define NUM_ENDGAME_AWARDS     20   ///< total number of endgame awards
#endif
#endif
#define NUMSHOW_ENDGAME_AWARDS 14   ///< number of awards to display that will fit on screen

#ifdef FEATURE_EDV
// used by demo_autotimescaleweapons;
#define ATSW_PANZER             0x01
#define ATSW_GRENADE            0x02
#define ATSW_DYNAMITE           0x04
#define ATSW_MORTAR             0x08
#define ATSW_SMOKE              0x10

// demo weapon cams
#define DWC_PANZER              0x01
#define DWC_MORTAR              0x02
#define DWC_GRENADE             0x04
#define DWC_DYNAMITE            0x08
#define DWC_SMOKE               0x10    ///< FIXME: add to demo control?

#define DEMO_NAMEOFF           0
#define DEMO_CLEANNAME         1
#define DEMO_COLOREDNAME       2

/**
 * @struct cam_s
 * @typedef cam_t
 * @brief
 */
typedef struct cam_s
{
	qboolean renderingFreeCam;
	qboolean renderingWeaponCam;
	qboolean wasRenderingWeaponCam;
	qboolean setCamAngles;              ///< Are we overriding angles via freecamSetPos

	vec3_t camAngle;                    ///< Stores the angle of our cam
	vec3_t camOrigin;                   ///< Stores the origin of our cam
	vec3_t velocity;

	qboolean startLean;

	int factor;
	qboolean noclip;

	int commandTime;

	int move;
	int turn;
} cam_t;

/**
 * @enum mlType_e
 * @brief ML_ = menu level
 * used in the demo helpmenu
 */
typedef enum
{
	ML_MAIN,
	ML_EDV
} mlType_t;
#endif

// limbopanel
#define SECONDARY_SLOT 0
#define PRIMARY_SLOT 1

/*
===============================================================================
LAGOMETER
===============================================================================
*/

#define PERIOD_SAMPLES 5000
#define LAG_SAMPLES     128
#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

/**
 * @struct
 * @typedef lagometer_t
 * @brief
 */
typedef struct
{
	int frameSamples[LAG_SAMPLES];
	int frameCount;
	int snapshotFlags[LAG_SAMPLES];
	int snapshotSamples[LAG_SAMPLES];
	int snapshotAntiwarp[LAG_SAMPLES];
	int snapshotCount;
} lagometer_t;

/**
 * @struct sample_s
 * @typedef sample_t
 * @brief
 */
typedef struct sample_s
{
	int elapsed;
	int time;
} sample_t;

/**
 * @struct sampledStat_s
 * @typedef sampledStat_t
 * @brief
 */
typedef struct sampledStat_s
{
	unsigned int count;
	int avg; // full int frames for output
	int lastSampleTime;
	sample_t samples[LAG_SAMPLES];
	int samplesTotalElpased;
} sampledStat_t;

/**
 * @struct sortedVotedMapByTotal_s
 * @typedef sortedVotedMapByTotal_s
 * @brief
 */
typedef struct sortedVotedMapByTotal_s
{
	int mapID;
	int totalVotes;
} sortedVotedMapByTotal_s;

/**
 * @struct cgs_s
 * @typedef cgs_t
 * @brief The client game static (cgs) structure should hold everything
 * loaded or calculated from the gamestate.  It will NOT
 * be cleared when a tournement restart is done, allowing
 * all clients to begin playing instantly
 */
typedef struct cgs_s
{
	gameState_t gameState;                          ///< gamestate from server
	glconfig_t glconfig;                            ///< rendering configuration
	float screenXScale;                             ///< derived from glconfig
	float screenYScale;
	float screenXBias;

	int serverCommandSequence;                      ///< reliable command stream counter
	int processedSnapshotNum;                       ///< the number of snapshots cgame has requested

	qboolean localServer;                           ///< detected on startup by checking sv_running

	// parsed from serverinfo
	gametype_t gametype;
	int antilag;

	float timelimit;
	int maxclients;
	char mapname[MAX_QPATH];
	char rawmapname[MAX_QPATH];
	float weaponRestrictions;

	int voteTime;
	int voteYes;
	int voteNo;
	qboolean voteModified;                          ///< beep whenever changed
	char voteString[MAX_STRING_TOKENS];

	int levelStartTime;
	int intermissionStartTime;

	// locally derived information from gamestate
	qhandle_t gameModels[MAX_MODELS];
	char gameShaderNames[MAX_CS_SHADERS][MAX_QPATH];
	qhandle_t gameShaders[MAX_CS_SHADERS];
	qhandle_t gameModelSkins[MAX_CS_SKINS];
	bg_character_t *gameCharacters[MAX_CHARACTERS];
	sfxHandle_t gameSounds[MAX_SOUNDS];
	sfxHandle_t cachedSounds[GAMESOUND_MAX];        ///< static game sounds

	int numInlineModels;
	qhandle_t inlineDrawModel[MAX_MODELS];
	vec3_t inlineModelMidpoints[MAX_MODELS];

	clientInfo_t clientinfo[MAX_CLIENTS];

	// teamchat width is *3 because of embedded color codes
	char teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH * 3 + 1];
	int teamChatMsgTimes[TEAMCHAT_HEIGHT];
	team_t teamChatMsgTeams[TEAMCHAT_HEIGHT];
	int teamChatPos;
	int teamLastChatPos;

	// New notify mechanism for obits
	char notifyMsgs[NOTIFY_HEIGHT][NOTIFY_WIDTH * 3 + 1];
	//int notifyMsgTimes[NOTIFY_HEIGHT];
	int notifyPos;
	int notifyLastPos;

	int cursorX;
	int cursorY;
	int eventHandling;

	// screen fading
	float fadeAlpha, fadeAlphaCurrent;
	int fadeStartTime;
	int fadeDuration;

	// media
	cgMedia_t media;

	// player/AI model scripting (client repository)
	animScriptData_t animScriptData;

	int currentRound;
	float nextTimeLimit;
	int minclients;
	gamestate_t gamestate;
	char *currentCampaign;
	int currentCampaignMap;

	int complaintClient;
	int complaintEndTime;

	playerStats_t playerStats;
	int numOIDtriggers;
	int teamobjectiveStats[MAX_OID_TRIGGERS];

	qboolean campaignInfoLoaded;
	cg_campaignInfo_t campaignData;

	qboolean arenaInfoLoaded;
	arenaInfo_t arenaData;

	centity_t *gameManager;

	int ccLayers;
	int ccLayerCeils[MAX_COMMANDMAP_LAYERS];
	float ccZoomFactor;

	int invitationClient;
	int invitationEndTime;

	int applicationClient;
	int applicationEndTime;

	int propositionClient;
	int propositionClient2;
	int propositionEndTime;

	int autoFireteamEndTime;
	int autoFireteamNum;

	int autoFireteamCreateEndTime;
	int autoFireteamCreateNum;

	int autoFireteamJoinEndTime;
	int autoFireteamJoinNum;

	qboolean autoMapExpanded;
	int autoMapExpandTime;

	qboolean autoMapOff;                                ///< is automap on or off

	int aviDemoRate;                                    ///< Demo playback recording
	int aReinfOffset[TEAM_NUM_TEAMS];                   ///< Team reinforcement offsets
	int cursorUpdate;                                   ///< Timeout for mouse pointer view
	fileHandle_t dumpStatsFile;                         ///< File to dump stats
	char *dumpStatsFileName;                            ///< Name of file to dump stats
	int dumpStatsTime;                                  ///< Next stats command that comes back will be written to a logfile
	int game_versioninfo;                               ///< game base version
	gameStats_t gamestats;
	topshotStats_t topshots;
	objectives_t objectives;
	qboolean fResize;                                   ///< MV window "resize" status
	qboolean fSelect;                                   ///< MV window "select" status
	qboolean fKeyPressed[MAX_KEYS];                     ///< Key status to get around console issues
	int timescaleUpdate;                                ///< Timescale display for demo playback
	int thirdpersonUpdate;

	cg_gamemodel_t miscGameModels[MAX_STATIC_GAMEMODELS];
	cg_corona_t corona[MAX_GAMECORONAS];

	vec2_t ccMenuPos;
	qboolean ccMenuShowing;
	int ccMenuType;
	mapEntityData_t ccMenuEnt;
	int ccSelectedLayer;
	int ccSelectedObjective;
	int ccSelectedSpawnPoint;
	int ccSelectedTeam;                                 ///< ( 1 = ALLIES, 0 = AXIS )
	int ccSelectedWeaponSlot;                           ///< ( 0 = secondary, 1 = primary)
	int ccSelectedClass;
	weapon_t ccSelectedPrimaryWeapon;                   ///< Selected primary weapon from limbo panel
	weapon_t ccSelectedSecondaryWeapon;                 ///< Selected secondary weapon from limbo panel
	int ccWeaponShots;
	int ccWeaponHits;
	vec3_t ccPortalPos;
	vec3_t ccPortalAngles;
	int ccPortalEnt;
	int ccFilter;
	int ccCurrentCamObjective;
	int ccRequestedObjective;
	int ccLastObjectiveRequestTime;

	int dbSortedClients[MAX_CLIENTS];
	int dbSelectedClient;

	int dbMode;
	qboolean dbShowing;
	qboolean dbAccuraciesReceived;
	qboolean dbPlayerKillsDeathsReceived;
	qboolean dbPlayerTimeReceived;
#ifdef FEATURE_RATING
	qboolean dbSkillRatingReceived;
#endif
#ifdef FEATURE_PRESTIGE
	qboolean dbPrestigeReceived;
#endif
	qboolean dbWeaponStatsReceived;
	qboolean dbLastScoreReceived;
	qboolean dbAwardsParsed;
	char *dbAwardNames[NUM_ENDGAME_AWARDS];
	team_t dbAwardTeams[NUM_ENDGAME_AWARDS];
	char dbAwardNamesBuffer[1024];
	int dbAwardsListOffset;
	int dbLastRequestTime;
	int dbPlayerListOffset;
	int dbWeaponListOffset;
	cg_weaponstats_t dbWeaponStats[WS_MAX];
	int dbHitRegions[HR_NUM_HITREGIONS];
	int dbChatMode;

	int tdbAxisMapsXP[SK_NUM_SKILLS][MAX_MAPS_PER_CAMPAIGN];
	int tdbAlliedMapsXP[SK_NUM_SKILLS][MAX_MAPS_PER_CAMPAIGN];
	int tdbMapListOffset;
	int tdbSelectedMap;

	int ftMenuPos;
	int ftMenuMode;
	int ftMenuModeEx;

	qboolean limboLoadoutSelected;
	qboolean limboLoadoutModified;

	oidInfo_t oidInfo[MAX_OID_TRIGGERS];

	qboolean initing;

	location_t location[MAX_C_LOCATIONS];
	int numLocations;
	qboolean locationsLoaded;
	clientLocation_t clientLocation[MAX_CLIENTS];

	// screen adjustments
	float adr43;                        ///< aspectratio / RATIO43
	float r43da;                        ///< RATIO43 / aspectratio
	float wideXoffset;                  ///< the x-offset for displaying horizontally centered loading/limbo screens

	// MAPVOTE
	int dbMapVoteListOffset;
	int dbNumMaps;
	char dbMaps[MAX_VOTE_MAPS][MAX_QPATH];
	char dbMapDispName[MAX_VOTE_MAPS][128];
	int dbMapVotes[MAX_VOTE_MAPS];
	int dbMapVotesSum;
	int dbMapID[MAX_VOTE_MAPS];
	int dbMapLastPlayed[MAX_VOTE_MAPS];
	int dbMapTotalVotes[MAX_VOTE_MAPS];
	int dbSelectedMap;
	int dbSelectedMapTime;
	qboolean dbMapListReceived;
	qboolean dbVoteTallyReceived;
	qboolean dbMapMultiVote;
	int dbMapVotedFor[3];
	sortedVotedMapByTotal_s dbSortedVotedMapsByTotal[MAX_VOTE_MAPS];
	int mapVoteMapX;
	int mapVoteMapY;

	int fixedphysics;
	int fixedphysicsfps;
	int pronedelay;
#ifdef FEATURE_RATING
	int skillRating;
	float mapProb;
#endif
#ifdef FEATURE_PRESTIGE
	int prestige;
#endif
#ifdef FEATURE_MULTIVIEW
	int mvAllowed;
#endif
#ifdef FEATURE_EDV
	cam_t demoCamera;
	mlType_t currentMenuLevel;
#endif

	int playerHitBoxHeight;

	qboolean sv_cheats;         // server allows cheats
	int sv_fps;                 // FPS server wants to send
	sampledStat_t sampledStat;  // fps client sample data

} cgs_t;

//==============================================================================

extern cgs_t        cgs;
extern cg_t         cg;
extern centity_t    cg_entities[MAX_GENTITIES];
extern weaponInfo_t cg_weapons[MAX_WEAPONS];
extern markPoly_t   cg_markPolys[MAX_MARK_POLYS];

extern vmCvar_t cg_centertime;
extern vmCvar_t cg_bobbing;

extern vmCvar_t cg_swingSpeed;
extern vmCvar_t cg_shadows;
extern vmCvar_t cg_gibs;
extern vmCvar_t cg_draw2D;
extern vmCvar_t cg_drawFPS;
extern vmCvar_t cg_drawPing;
extern vmCvar_t cg_lagometer;
extern vmCvar_t cg_drawSnapshot;
extern vmCvar_t cg_drawCrosshair;
extern vmCvar_t cg_drawCrosshairInfo;
extern vmCvar_t cg_drawCrosshairNames;
extern vmCvar_t cg_drawCrosshairPickups;
extern vmCvar_t cg_drawSpectatorNames;
extern vmCvar_t cg_useWeapsForZoom;
extern vmCvar_t cg_weaponCycleDelay;
extern vmCvar_t cg_cycleAllWeaps;
extern vmCvar_t cg_crosshairX;
extern vmCvar_t cg_crosshairY;
extern vmCvar_t cg_crosshairSize;
extern vmCvar_t cg_crosshairHealth;
extern vmCvar_t cg_drawStatus;
extern vmCvar_t cg_animSpeed;
extern vmCvar_t cg_debugAnim;
extern vmCvar_t cg_debugPosition;
extern vmCvar_t cg_debugEvents;
extern vmCvar_t cg_drawSpreadScale;
extern vmCvar_t cg_railTrailTime;
extern vmCvar_t cg_errorDecay;
extern vmCvar_t cg_nopredict;
extern vmCvar_t cg_noPlayerAnims;
extern vmCvar_t cg_showmiss;
extern vmCvar_t cg_markTime;
extern vmCvar_t cg_brassTime;
extern vmCvar_t cg_gun_frame;
extern vmCvar_t cg_gun_x;
extern vmCvar_t cg_gun_y;
extern vmCvar_t cg_gun_z;
extern vmCvar_t cg_drawGun;
extern vmCvar_t cg_cursorHints;
extern vmCvar_t cg_letterbox;
extern vmCvar_t cg_tracerChance;
extern vmCvar_t cg_tracerWidth;
extern vmCvar_t cg_tracerLength;
extern vmCvar_t cg_tracerSpeed;
extern vmCvar_t cg_autoswitch;
extern vmCvar_t cg_fov;
extern vmCvar_t cg_muzzleFlash;
extern vmCvar_t cg_drawEnvAwareness;

extern vmCvar_t cg_zoomDefaultSniper;

extern vmCvar_t cg_zoomStepSniper;
extern vmCvar_t cg_thirdPersonRange;
extern vmCvar_t cg_thirdPersonAngle;
extern vmCvar_t cg_thirdPerson;
#ifdef ALLOW_GSYNC
extern vmCvar_t cg_synchronousClients;
#endif // ALLOW_GSYNC
extern vmCvar_t cg_teamChatTime;
extern vmCvar_t cg_teamChatHeight;
extern vmCvar_t cg_teamChatMention;
extern vmCvar_t cg_stats;
extern vmCvar_t cg_coronafardist;
extern vmCvar_t cg_coronas;
extern vmCvar_t cg_buildScript;
extern vmCvar_t cg_paused;
extern vmCvar_t cg_blood;
extern vmCvar_t cg_predictItems;
extern vmCvar_t cg_teamChatsOnly;
extern vmCvar_t cg_voiceChats;
extern vmCvar_t cg_voiceText;
extern vmCvar_t cg_autoactivate;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;

extern vmCvar_t cg_timescale;

extern vmCvar_t cg_voiceSpriteTime;

extern vmCvar_t cg_gameType;
extern vmCvar_t cg_bloodTime;
extern vmCvar_t cg_skybox;

extern vmCvar_t cg_redlimbotime;
extern vmCvar_t cg_bluelimbotime;

extern vmCvar_t cg_movespeed;

extern vmCvar_t cg_drawCompass;
extern vmCvar_t cg_drawNotifyText;
extern vmCvar_t cg_quickMessageAlt;

extern vmCvar_t cg_descriptiveText;

extern vmCvar_t cg_antilag;

extern vmCvar_t developer;

extern vmCvar_t authLevel;
extern vmCvar_t cf_wstats;
extern vmCvar_t cf_wtopshots;
extern vmCvar_t cg_autoFolders;
extern vmCvar_t cg_autoAction;
extern vmCvar_t cg_autoReload;
extern vmCvar_t cg_bloodDamageBlend;
extern vmCvar_t cg_bloodFlash;
extern vmCvar_t cg_bloodFlashTime;
extern vmCvar_t cg_complaintPopUp;
extern vmCvar_t cg_crosshairAlpha;
extern vmCvar_t cg_crosshairAlphaAlt;
extern vmCvar_t cg_crosshairColor;
extern vmCvar_t cg_crosshairColorAlt;
extern vmCvar_t cg_crosshairPulse;
extern vmCvar_t cg_drawReinforcementTime;
extern vmCvar_t cg_drawWeaponIconFlash;
extern vmCvar_t cg_noAmmoAutoSwitch;
extern vmCvar_t cg_printObjectiveInfo;
#ifdef FEATURE_MULTIVIEW
extern vmCvar_t cg_specHelp;
#endif
extern vmCvar_t cg_uinfo;

extern vmCvar_t demo_avifpsF1;
extern vmCvar_t demo_avifpsF2;
extern vmCvar_t demo_avifpsF3;
extern vmCvar_t demo_avifpsF4;
extern vmCvar_t demo_avifpsF5;
extern vmCvar_t demo_drawTimeScale;
extern vmCvar_t demo_infoWindow;
#ifdef FEATURE_MULTIVIEW
extern vmCvar_t mv_sensitivity;
#endif
#ifdef FEATURE_EDV
extern vmCvar_t demo_weaponcam;
extern vmCvar_t demo_followDistance;
extern vmCvar_t demo_yawPitchRollSpeed;
extern vmCvar_t demo_lookat;
extern vmCvar_t demo_teamonlymissilecam;
extern vmCvar_t demo_autotimescale;
extern vmCvar_t demo_autotimescaleweapons;
extern vmCvar_t demo_freecamspeed;
extern vmCvar_t demo_nopitch;
extern vmCvar_t demo_pvshint;
extern vmCvar_t cg_predefineddemokeys;
#endif
// engine mappings
extern vmCvar_t int_cl_maxpackets;
extern vmCvar_t int_cl_timenudge;
extern vmCvar_t int_m_pitch;
extern vmCvar_t int_sensitivity;
extern vmCvar_t int_ui_blackout;

extern vmCvar_t cg_rconPassword;
extern vmCvar_t cg_refereePassword;
extern vmCvar_t cg_atmosphericEffects;

extern vmCvar_t cg_drawRoundTimer;

extern vmCvar_t cg_debugSkills;
extern vmCvar_t cg_drawFireteamOverlay;
extern vmCvar_t cg_drawSmallPopupIcons;

// some optimization cvars
extern vmCvar_t cg_instanttapout;

// demo recording cvars
extern vmCvar_t cl_demorecording;
extern vmCvar_t cl_demofilename;
extern vmCvar_t cl_demooffset;
extern vmCvar_t cl_waverecording;
extern vmCvar_t cl_wavefilename;
extern vmCvar_t cl_waveoffset;
extern vmCvar_t cg_recording_statusline;

extern vmCvar_t cg_announcer;
extern vmCvar_t cg_hitSounds;
extern vmCvar_t cg_locations;

extern vmCvar_t cg_spawnTimer_period;
extern vmCvar_t cg_spawnTimer_set;

extern vmCvar_t cg_countryflags; ///< GeoIP

extern vmCvar_t cg_altHud;
extern vmCvar_t cg_altHudFlags;
extern vmCvar_t cg_tracers;
extern vmCvar_t cg_fireteamLatchedClass;
extern vmCvar_t cg_simpleItems;

extern vmCvar_t cg_weapaltReloads;

extern vmCvar_t cg_automapZoom;

extern vmCvar_t cg_drawTime;

extern vmCvar_t cg_popupFadeTime;
extern vmCvar_t cg_popupStayTime;
extern vmCvar_t cg_popupFilter;
extern vmCvar_t cg_popupBigFilter;
extern vmCvar_t cg_graphicObituaries;

extern vmCvar_t cg_fontScaleTP;
extern vmCvar_t cg_fontScaleSP;
extern vmCvar_t cg_fontScaleCP;
extern vmCvar_t cg_fontScaleCN;

// unlagged optimized prediction
extern vmCvar_t cg_optimizePrediction;
extern vmCvar_t cg_debugPlayerHitboxes;

// scoreboard
extern vmCvar_t cg_scoreboard;

#define SCOREBOARD_XP    0
#define SCOREBOARD_SR    1
#define SCOREBOARD_PR    2

extern vmCvar_t cg_quickchat;

extern vmCvar_t cg_drawspeed;

extern vmCvar_t cg_visualEffects;  ///< turn invisible (0) / visible (1) visual effect (i.e airstrike plane, debris ...)
extern vmCvar_t cg_bannerTime;

extern vmCvar_t cg_shoutcastDrawPlayers;
extern vmCvar_t cg_shoutcastDrawTeamNames;
extern vmCvar_t cg_shoutcastTeamName1;
extern vmCvar_t cg_shoutcastTeamName2;
extern vmCvar_t cg_shoutcastDrawHealth;
extern vmCvar_t cg_shoutcastGrenadeTrail;
extern vmCvar_t cg_shoutcastDrawMinimap;

// local clock flags
#define LOCALTIME_ON                0x01
#define LOCALTIME_SECOND            0x02
#define LOCALTIME_12HOUR            0x04

// crosshair name flags
#define CROSSHAIR_CLASS             0x01
#define CROSSHAIR_RANK              0x02
#ifdef FEATURE_PRESTIGE
#define CROSSHAIR_PRESTIGE          0x04
#endif

// projectile spawn effects at destination
#define PS_FX_NONE   0
#define PS_FX_COMMON 1
#define PS_FX_WATER  2
#define PS_FX_FLESH  3

// cg_atmospheric.c
void CG_EffectParse(const char *effectstr);
void CG_AddAtmosphericEffects(void);

// cg_character.c
qboolean CG_RegisterCharacter(const char *characterFile, bg_character_t *character);
bg_character_t *CG_CharacterForClientinfo(clientInfo_t *ci, centity_t *cent);
bg_character_t *CG_CharacterForPlayerstate(playerState_t *ps);
void CG_RegisterPlayerClasses(void);

// cg_main.c
const char *CG_ConfigString(int index);
int CG_ConfigStringCopy(int index, char *buff, size_t buffsize);
const char *CG_Argv(int arg);

float CG_Cvar_Get(const char *cvar);

char *CG_generateFilename(void);
int CG_findClientNum(const char *s);
void CG_printConsoleString(const char *str);

void CG_LoadObjectiveData(void);

void QDECL CG_Printf(const char *msg, ...) _attribute((format(printf, 1, 2)));
void QDECL CG_DPrintf(const char *msg, ...) _attribute((format(printf, 1, 2)));
void QDECL CG_Error(const char *msg, ...) _attribute((noreturn, format(printf, 1, 2)));

void CG_StartMusic(void);
void CG_QueueMusic(void);

void CG_UpdateCvars(void);

qboolean CG_execFile(const char *filename);
int CG_CrosshairPlayer(void);
int CG_LastAttacker(void);
void CG_KeyEvent(int key, qboolean down);
void CG_MouseEvent(int x, int y);
void CG_EventHandling(int type, qboolean fForced);

qboolean CG_GetTag(int clientNum, const char *tagname, orientation_t *orientation);
qboolean CG_GetWeaponTag(int clientNum, const char *tagname, orientation_t *orientation);

sfxHandle_t CG_GetGameSound(int index);

void QDECL CG_WriteToLog(const char *fmt, ...) _attribute((format(printf, 1, 2)));

int CG_cleanName(const char *pszIn, char *pszOut, int dwMaxLength, qboolean fCRLF);

// cg_view.c
void CG_TestModel_f(void);
void CG_TestGun_f(void);
void CG_TestModelNextFrame_f(void);
void CG_TestModelPrevFrame_f(void);
void CG_TestModelNextSkin_f(void);
void CG_TestModelPrevSkin_f(void);
void CG_ZoomIn_f(void);
void CG_ZoomOut_f(void);

void CG_SetupFrustum(void);
qboolean CG_CullPoint(vec3_t pt);
qboolean CG_CullPointAndRadius(const vec3_t pt, vec_t radius);

void CG_DrawActiveFrame(int serverTime, qboolean demoPlayback);
void CG_DrawSkyBoxPortal(qboolean fLocalView);

void CG_Letterbox(float xsize, float ysize, qboolean center);

// cg_drawtools.c

qboolean Ccg_Is43Screen(void);      // does this game-window have a 4:3 aspectratio. note: this is also true for a 800x600 windowed game on a widescreen monitor
float Ccg_WideX(float x);           // convert an x-coordinate to a widescreen x-coordinate. (only if the game-window is non 4:3 aspectratio)
float Ccg_WideXoffset(void);        // the horizontal center of screen pixel-difference of a 4:3 ratio vs. the current aspectratio
void CG_AdjustFrom640(float *x, float *y, float *w, float *h);
void CG_FillRect(float x, float y, float width, float height, const float *color);
void CG_HorizontalPercentBar(float x, float y, float width, float height, float percent);
void CG_DrawPic(float x, float y, float width, float height, qhandle_t hShader);
void CG_DrawPicST(float x, float y, float width, float height, float s0, float t0, float s1, float t1, qhandle_t hShader);
void CG_DrawRotatedPic(float x, float y, float width, float height, qhandle_t hShader, float angle);        // NERVE - SMF
void CG_FilledBar(float x, float y, float w, float h, float *startColor, float *endColor, const float *bgColor, float frac, int flags);

void CG_DrawStretchPic(float x, float y, float width, float height, qhandle_t hShader);

float *CG_FadeColor(int startMsec, int totalMsec);
float *CG_LerpColorWithAttack(vec4_t from, vec4_t to, int startMsec, int totalMsec, int attackMsec);
float *CG_TeamColor(int team);
void CG_TileClear(void);
void CG_ColorForHealth(vec4_t hcolor);
void CG_GetColorForHealth(int health, vec4_t hcolor);

qboolean CG_WorldCoordToScreenCoordFloat(vec3_t point, float *x, float *y);
void CG_AddOnScreenText(const char *text, vec3_t origin);
void CG_AddOnScreenBar(float fraction, vec4_t colorStart, vec4_t colorEnd, vec4_t colorBack, vec3_t origin);

// string word wrapper
char *CG_WordWrapString(const char *input, int maxLineChars, char *output, int maxOutputSize);
// draws multiline strings
void CG_DrawMultilineText(float x, float y, float scalex, float scaley, vec4_t color, const char *text, int lineHeight, float adjust, int limit, int style, int align, fontHelper_t *font);

// new hud stuff
void CG_DrawRect(float x, float y, float width, float height, float size, const float *color);
void CG_DrawRect_FixedBorder(float x, float y, float width, float height, int border, const float *color);
void CG_DrawSides(float x, float y, float w, float h, float size);
void CG_DrawTopBottom(float x, float y, float w, float h, float size);
void CG_DrawTopBottom_NoScale(float x, float y, float w, float h, float size);
void CG_DrawBottom_NoScale(float x, float y, float w, float h, float size);

// localization functions
const char *CG_TranslateString(const char *string);

void CG_InitStatsDebug(void);
void CG_StatsDebugAddText(const char *text);
qhandle_t CG_GetCompassIcon(centity_t *cent, qboolean drawVoicesChat, qboolean drawFireTeam);
void CG_DrawCompassIcon(float x, float y, float w, float h, vec3_t origin, vec3_t dest, qhandle_t shader, float dstScale, float baseSize);

void CG_AddLagometerFrameInfo(void);
void CG_AddLagometerSnapshotInfo(snapshot_t *snap);
void CG_CenterPrint(const char *str, int y, float fontScale);
void CG_PriorityCenterPrint(const char *str, int y, float fontScale, int priority);
void CG_ObjectivePrint(const char *str, float fontScale);
void CG_DrawActive(void);
void CG_CheckForCursorHints(void);
void CG_DrawTeamBackground(int x, int y, int w, int h, float alpha, int team);
void CG_Text_Paint_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontHelper_t *font);
void CG_Text_Paint_Centred_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontHelper_t *font);
void CG_Text_Paint_RightAligned_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontHelper_t *font);
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style);
void CG_Text_PaintWithCursor_Ext(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, const char *cursor, int limit, int style, fontHelper_t *font);
void CG_Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, const char *cursor, int limit, int style);
void CG_Text_SetActiveFont(int font);
int CG_Text_Width_Ext(const char *text, float scale, int limit, fontHelper_t *font);
int CG_Text_Width(const char *text, float scale, int limit);
int CG_Text_Height_Ext(const char *text, float scale, int limit, fontHelper_t *font);
int CG_Text_Height(const char *text, float scale, int limit);
float CG_GetValue(int ownerDraw, int type);   // 'type' is relative or absolute (fractional-'0.5' or absolute- '50' health)
qboolean CG_OwnerDrawVisible(int flags);
void CG_RunMenuScript(char **args);
void CG_GetTeamColor(vec4_t *color);

// cg_draw_hud.c
void CG_ReadHudScripts(void);
void CG_Hud_Setup(void);
void CG_DrawUpperRight(void);
void CG_SetHud(void);
void CG_DrawActiveHud(void);
void CG_DrawGlobalHud(void);

void CG_Text_PaintChar_Ext(float x, float y, float w, float h, float scalex, float scaley, float s, float t, float s2, float t2, qhandle_t hShader);
void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader);

void CG_DrawCursorhint(rectDef_t *rect);
void CG_DrawWeapStability(rectDef_t *rect);
void CG_DrawWeapHeat(rectDef_t *rect, int align);
void CG_DrawPlayerWeaponIcon(rectDef_t *rect, qboolean drawHighlighted, int align, vec4_t *refcolor);
int CG_CalculateReinfTime(qboolean menu);
float CG_CalculateReinfTime_Float(qboolean menu);
int CG_CalculateShoutcasterReinfTime(team_t team);
void CG_Fade(int r, int g, int b, int a, int time, int duration);

void CG_PlayerAmmoValue(int *ammo, int *clips, int *akimboammo);

//cg_shoutcastoverlay.c

void CG_DrawShoutcastPlayerList(void);
void CG_DrawShoutcastPlayerStatus(void);
void CG_DrawShoutcastTimer(void);
void CG_DrawShoutcastPowerups(void);
void CG_DrawMinimap(void);

void CG_Shoutcast_KeyHandling(int key, qboolean down);
qboolean CG_ShoutcastCheckExecKey(int key, qboolean doaction);

// cg_player.c
qboolean CG_EntOnFire(centity_t *cent);
void CG_Player(centity_t *cent);
void CG_ResetPlayerEntity(centity_t *cent);
void CG_AddRefEntityWithPowerups(refEntity_t *ent, int powerups, int team, entityState_t *es, const vec3_t fireRiseDir);
void CG_NewClientInfo(int clientNum);
sfxHandle_t CG_CustomSound(int clientNum, const char *soundName);
void CG_ParseTeamXPs(int n);

int CG_GetPlayerMaxHealth(int clientNum, int class, int team);

// cg_predict.c
void CG_BuildSolidList(void);
int CG_PointContents(const vec3_t point, int passEntityNum);
void CG_Trace(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask);
void CG_PredictPlayerState(void);

// cg_edv.c
void CG_RunBindingBuf(int key, qboolean down, char *buf);
void CG_RunBinding(int key, qboolean down);
void CG_EDV_WeaponCam(centity_t *cent, refEntity_t *ent);
void CG_EDV_RunInput(void);

// cg_events.c
void CG_CheckEvents(centity_t *cent);
void CG_EntityEvent(centity_t *cent, vec3_t position);
void CG_PainEvent(centity_t *cent, int health, qboolean crouching);
void CG_PrecacheFXSounds(void);

// cg_ents.c
void CG_SetEntitySoundPosition(centity_t *cent);
void CG_AddPacketEntities(void);
void CG_Beam(centity_t *cent);
void CG_AdjustPositionForMover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t outDeltaAngles);
void CG_AddCEntity(centity_t *cent);
qboolean CG_AddCEntity_Filter(centity_t *cent);
qboolean CG_AddLinkedEntity(centity_t *cent, qboolean ignoreframe, int atTime);
void CG_PositionEntityOnTag(refEntity_t *entity, const refEntity_t *parent, const char *tagName, int startIndex, vec3_t *offset);
void CG_PositionRotatedEntityOnTag(refEntity_t *entity, const refEntity_t *parent, const char *tagName);

// cg_weapons.c
void CG_LastWeaponUsed_f(void);
void CG_NextWeaponInBank_f(void);
void CG_PrevWeaponInBank_f(void);
void CG_AltWeapon_f(void);
void CG_NextWeapon_f(void);
void CG_PrevWeapon_f(void);
void CG_Weapon_f(void);
void CG_WeaponBank_f(void);
qboolean CG_WeaponSelectable(int weapon);

void CG_FinishWeaponChange(int lastweap, int newweap);
void CG_SetSniperZoom(int weapon);

void CG_RegisterWeapon(int weaponNum, qboolean force);
void CG_RegisterItemVisuals(int itemNum);

void CG_FireWeapon(centity_t *cent);

void CG_MissileHitWall(int weapon, int missileEffect, vec3_t origin, vec3_t dir, int surfFlags, int entHit);     // modified to send missilehitwall surface parameters

void CG_MissileHitWallSmall(vec3_t origin, vec3_t dir);
void CG_DrawTracer(const vec3_t start, const vec3_t finish);

void CG_MG42EFX(centity_t *cent);
void CG_MortarEFX(centity_t *cent);

void CG_MissileHitPlayer(int entityNum, int weapon, vec3_t origin, vec3_t dir, int fleshEntityNum);
qboolean CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle);
void CG_Bullet(int weapon, vec3_t end, int sourceEntityNum, qboolean flesh, int fleshEntityNum, int otherEntNum2, float waterfraction, int seed);

void CG_RailTrail(vec3_t color, vec3_t start, vec3_t end, int type, int index);
void CG_RailTrail2(vec3_t color, vec3_t start, vec3_t end, int index, int sideNum);

void CG_AddViewWeapon(playerState_t *ps);
void CG_AddPlayerWeapon(refEntity_t *parent, playerState_t *ps, centity_t *cent);
void CG_AddSoundWeapon(centity_t *cent);

void CG_OutOfAmmoChange(qboolean allowforceswitch);

soundSurface_t CG_GetSoundSurfaceIndex(int surfFlags);
sfxHandle_t CG_GetRandomSoundSurface(weaponSounds_t *weaponSounds, soundSurface_t surf, qboolean forceDefault);

// added to header to access from outside cg_weapons.c
void CG_AddDebris(vec3_t origin, vec3_t dir, int speed, int duration, int count, trace_t *trace);

// cg_marks.c
void CG_InitMarkPolys(void);
void CG_AddMarks(void);
void CG_ImpactMark(qhandle_t markShader,
                   vec3_t origin, vec4_t projection, float radius, float orientation,
                   float r, float g, float b, float a, int lifeTime);

// cg_particles.c
void CG_ClearParticles(void);
void CG_InitParticles(void);
void CG_AddParticles(void);
void CG_ParticleSnow(qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum);
void CG_ParticleSmoke(qhandle_t pshader, centity_t *cent);
void CG_ParticleSnowFlurry(qhandle_t pshader, centity_t *cent);
void CG_ParticleBulletDebris(vec3_t org, vec3_t vel, int duration);
void CG_ParticleDirtBulletDebris_Core(vec3_t org, vec3_t vel, int duration, float width, float height, float alpha, qhandle_t shader);
void CG_ParticleSparks(vec3_t org, vec3_t vel, int duration, float x, float y, float speed);
void CG_ParticleDust(centity_t *cent, vec3_t origin, vec3_t dir);

void CG_ParticleExplosion(const char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd, qboolean dlight);

void CG_ParticleImpactSmokePuff(qhandle_t pshader, vec3_t origin);
void CG_ParticleImpactSmokePuffExtended(qhandle_t pshader, vec3_t origin, int lifetime, int vel, int acc, int maxroll, float alpha, float size);        // so I can add more parameters without screwing up the one that's there
#ifdef BLOOD_PARTICLE_TRAIL
void CG_Particle_Bleed(qhandle_t pshader, vec3_t start, vec3_t dir, int fleshEntityNum, int duration);
#endif
void CG_GetBleedOrigin(vec3_t head_origin, vec3_t body_origin, int fleshEntityNum);
void CG_Particle_OilParticle(qhandle_t pshader, vec3_t origin, vec3_t dir, int ptime, int snum);
void CG_Particle_OilSlick(qhandle_t pshader, centity_t *cent);
void CG_OilSlickRemove(centity_t *cent);
void CG_ParticleBloodCloud(centity_t *cent, vec3_t origin, vec3_t dir);

// cg_trails.c
// usedby for zinx's trail fixes
int CG_AddTrailJunc(int headJuncIndex, void *usedby, qhandle_t shader, int spawnTime, int sType, vec3_t pos, int trailLife, float alphaStart, float alphaEnd, float startWidth, float endWidth, int flags, vec3_t colorStart, vec3_t colorEnd, float sRatio, float animSpeed);
int CG_AddSparkJunc(int headJuncIndex, void *usedby, qhandle_t shader, vec3_t pos, int trailLife, float alphaStart, float alphaEnd, float startWidth, float endWidth);
int CG_AddSmokeJunc(int headJuncIndex, void *usedby, qhandle_t shader, vec3_t pos, int trailLife, float alpha, float startWidth, float endWidth);
void CG_AddTrails(void);
void CG_ClearTrails(void);

// cg_sound.c
// sound scripting
int CG_SoundScriptPrecache(const char *name);
int CG_SoundPlaySoundScript(const char *name, vec3_t org, int entnum, qboolean buffer);
void CG_UpdateBufferedSoundScripts(void);
// prototype must match animScriptData_t::playSound
void CG_SoundPlayIndexedScript(int index, vec3_t org, int entnum);
void CG_SoundInit(void);

void CG_SetViewanglesForSpeakerEditor(void);
void CG_SpeakerEditorDraw(void);
void CG_SpeakerEditor_KeyHandling(int key, qboolean down);
void CG_Debriefing_KeyEvent(int key, qboolean down);
void CG_SpeakerEditorMouseMove_Handling(int x, int y);
void CG_ActivateEditSoundMode(void);
void CG_DeActivateEditSoundMode(void);
void CG_ModifyEditSpeaker(void);
void CG_UndoEditSpeaker(void);
void CG_ToggleActiveOnScriptSpeaker(int index);
void CG_UnsetActiveOnScriptSpeaker(int index);
void CG_SetActiveOnScriptSpeaker(int index);
void CG_AddScriptSpeakers(void);

// flamethrower
void CG_FireFlameChunks(centity_t *cent, vec3_t origin, vec3_t angles, float speedScale, qboolean firing);
void CG_InitFlameChunks(void);
void CG_AddFlameChunks(void);
void CG_UpdateFlamethrowerSounds(void);

// cg_localents.c
void CG_InitLocalEntities(void);
localEntity_t *CG_AllocLocalEntity(void);
localEntity_t *CG_FindLocalEntity(int index, int sideNum);
void CG_AddLocalEntities(void);

char *CG_GetLocationMsg(int clientNum, vec3_t origin);
char *CG_BuildLocationString(int clientNum, vec3_t origin, int flag);
void CG_LoadLocations(void);

// cg_effects.c
int CG_GetOriginForTag(centity_t *cent, refEntity_t *parent, const char *tagName, int startIndex, vec3_t org, vec3_t axis[3]);
localEntity_t *CG_SmokePuff(const vec3_t p,
                            const vec3_t vel,
                            float radius,
                            float r, float g, float b, float a,
                            float duration,
                            int startTime,
                            int fadeInTime,
                            int leFlags,
                            qhandle_t hShader);

void CG_BubbleTrail(vec3_t start, vec3_t end, float size, float spacing);

void CG_GibPlayer(centity_t *cent, vec3_t playerOrigin, vec3_t gdir);
void CG_LoseHat(centity_t *cent, vec3_t dir);

void CG_Bleed(vec3_t origin, int entityNum);

localEntity_t *CG_MakeExplosion(vec3_t origin, vec3_t dir,
                                qhandle_t hModel, qhandle_t shader, int msec, qboolean isSprite);

void CG_SparklerSparks(vec3_t origin, int count);
void CG_ClearFlameChunks(void);

void CG_RumbleEfx(float pitch, float yaw);

void InitSmokeSprites(void);
void CG_RenderSmokeGrenadeSmoke(centity_t *cent, const weaponInfo_t *weapon);
void CG_AddSmokeSprites(void);

// cg_snapshot.c
void CG_ProcessSnapshots(void);

// cg_spawn.c
qboolean    CG_SpawnString(const char *key, const char *defaultString, char **out);
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean    CG_SpawnFloat(const char *key, const char *defaultString, float *out);
qboolean    CG_SpawnInt(const char *key, const char *defaultString, int *out);
qboolean    CG_SpawnVector(const char *key, const char *defaultString, float *out);
void        CG_ParseEntitiesFromString(void);

// cg_info.c
void CG_LoadingString(const char *s);
void CG_DrawInformation(qboolean forcerefresh);
void CG_DemoClick(int key, qboolean down);
void CG_ShowHelp_Off(int *status);
void CG_ShowHelp_On(int *status);
#ifdef FEATURE_MULTIVIEW
qboolean CG_ViewingDraw(void);
#endif

// cg_scoreboard.c
qboolean CG_DrawScoreboard(void);
int SkillNumForClass(int classNum);

void CG_TransformToCommandMapCoord(float *coord_x, float *coord_y);

void CG_DrawExpandedAutoMap(void);
void CG_DrawAutoMap(float x, float y, float w, float h);

qboolean CG_DrawMissionBriefing(void);
void CG_MissionBriefingClick(int key);

qboolean CG_DrawFlag(float x, float y, float fade, int clientNum);

// MAPVOTE
qboolean CG_FindArenaInfo(const char *filename, const char *mapname, arenaInfo_t *info);

void CG_LoadRankIcons(void);
qboolean CG_DrawStatsRanksMedals(void);
void CG_StatsRanksMedalsClick(int key);

void CG_ParseFireteams(void);
void CG_ParseOIDInfos(void);
//oidInfo_t *CG_OIDInfoForEntityNum(int num);

// cg_consolecmds.c
extern const char *aMonths[12];
qboolean CG_ConsoleCommand(void);
void CG_InitConsoleCommands(void);
void CG_ScoresDown_f(void);
void CG_ScoresUp_f(void);
void CG_autoRecord_f(void);
void CG_autoScreenShot_f(void);
void CG_keyOn_f(void);
void CG_keyOff_f(void);
void CG_dumpStats_f(void);

void CG_ForceTapOut_f(void);

// MAPVOTE
void CG_parseMapVoteListInfo(void);
void CG_parseMapVoteTally(void);

// cg_servercmds.c
void CG_ExecuteNewServerCommands(int latestSequence);
void CG_ParseServerinfo(void);
void CG_ParseSysteminfo(void);
void CG_ParseModInfo(void);
void CG_ParseWolfinfo(void);
void CG_ParseSpawns(void);
void CG_ParseServerVersionInfo(const char *pszVersionInfo);
void CG_ParseReinforcementTimes(const char *pszReinfSeedString);
void CG_SetConfigValues(void);
void CG_ShaderStateChanged(void);
void CG_ChargeTimesChanged(void);
void CG_TeamRestrictionsChanged(void);
void CG_SkillLevelsChanged(void);
void CG_LoadVoiceChats(void);
void CG_PlayBufferedVoiceChats(void);
void CG_AddToNotify(const char *str);
const char *CG_LocalizeServerCommand(const char *buf);
void CG_wstatsParse_cmd(void);

void CG_parseWeaponStats_cmd(void(txt_dump) (const char *));
//void CG_parseBestShotsStats_cmd(qboolean doTop, void(txt_dump) (const char *));
//void CG_parseTopShotsStats_cmd(qboolean doTop, void(txt_dump) (const char *));
//void CG_scores_cmd(void);

void CG_UpdateSvCvars(void);

/**
 * @struct consoleCommand_t
 * @brief
 */
typedef struct
{
	const char *cmd;
	void (*function)(void);
} consoleCommand_t;

// cg_playerstate.c
void CG_Respawn(qboolean revived);
void CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops);

//===============================================

// system traps
// These functions are how the cgame communicates with the main game system

// print message on the local console
void trap_Print(const char *fmt);

// abort the game
void trap_Error(const char *fmt) _attribute((noreturn));

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int trap_Milliseconds(void);
int trap_RealTime(qtime_t *qtime);

// console variable interaction
void trap_Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags);
void trap_Cvar_Update(vmCvar_t *vmCvar);
void trap_Cvar_Set(const char *varName, const char *value);
void trap_Cvar_VariableStringBuffer(const char *varName, char *buffer, int bufsize);
void trap_Cvar_LatchedVariableStringBuffer(const char *varName, char *buffer, int bufsize);

// ServerCommand and ConsoleCommand parameter access
int trap_Argc(void);
void trap_Argv(int n, char *buffer, int bufferLength);
void trap_Args(char *buffer, int bufferLength);

// filesystem access
// returns length of file
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void trap_FS_Read(void *buffer, int len, fileHandle_t f);
void trap_FS_Write(const void *buffer, int len, fileHandle_t f);
void trap_FS_FCloseFile(fileHandle_t f);
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
int trap_FS_Delete(const char *filename);

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void trap_SendConsoleCommand(const char *text);

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void trap_AddCommand(const char *cmdName);

void trap_RemoveCommand(const char *cmdName);

// send a string to the server over the network
void trap_SendClientCommand(const char *s);

// force a screen update, only used during gamestate load
void trap_UpdateScreen(void);

// model collision
void trap_CM_LoadMap(const char *mapname);
int  trap_CM_NumInlineModels(void);
clipHandle_t trap_CM_InlineModel(int index);        // 0 = world, 1+ = bmodels
clipHandle_t trap_CM_TempBoxModel(const vec3_t mins, const vec3_t maxs);
clipHandle_t trap_CM_TempCapsuleModel(const vec3_t mins, const vec3_t maxs);
int trap_CM_PointContents(const vec3_t p, clipHandle_t model);
int trap_CM_TransformedPointContents(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles);
void trap_CM_BoxTrace(trace_t *results, const vec3_t start, const vec3_t end,
                      const vec3_t mins, const vec3_t maxs,
                      clipHandle_t model, int brushmask);
void trap_CM_TransformedBoxTrace(trace_t *results, const vec3_t start, const vec3_t end,
                                 const vec3_t mins, const vec3_t maxs,
                                 clipHandle_t model, int brushmask,
                                 const vec3_t origin, const vec3_t angles);

void trap_CM_CapsuleTrace(trace_t *results, const vec3_t start, const vec3_t end,
                          const vec3_t mins, const vec3_t maxs,
                          clipHandle_t model, int brushmask);
void trap_CM_TransformedCapsuleTrace(trace_t *results, const vec3_t start, const vec3_t end,
                                     const vec3_t mins, const vec3_t maxs,
                                     clipHandle_t model, int brushmask,
                                     const vec3_t origin, const vec3_t angles);

// Returns the projection of a polygon onto the solid brushes in the world
int trap_CM_MarkFragments(int numPoints, const vec3_t *points,
                          const vec3_t projection,
                          int maxPoints, vec3_t pointBuffer,
                          int maxFragments, markFragment_t *fragmentBuffer);

// projects a decal onto brush model surfaces
void trap_R_ProjectDecal(qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime, int fadeTime);
void trap_R_ClearDecals(void);

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void trap_S_StartSound(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx);
void trap_S_StartSoundVControl(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int volume);
void trap_S_StartSoundEx(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int flags);
void trap_S_StartSoundExVControl(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int flags, int volume);
void trap_S_StopStreamingSound(int entityNum);    // usually AI.  character is talking and needs to be shut up /now/
int trap_S_GetSoundLength(sfxHandle_t sfx);
int trap_S_GetCurrentSoundTime(void);

// a local sound is always played full volume
void trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum);
void trap_S_ClearLoopingSounds(void);
void trap_S_ClearSounds(qboolean killmusic);
void trap_S_AddLoopingSound(const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int volume, int soundTime);
void trap_S_AddRealLoopingSound(const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int range, int volume, int soundTime);
void trap_S_UpdateEntityPosition(int entityNum, const vec3_t origin);

// talking animations
int trap_S_GetVoiceAmplitude(int entityNum);

// respatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void trap_S_Respatialize(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater);
sfxHandle_t trap_S_RegisterSound(const char *sample, qboolean compressed);          // returns blank if not found
void trap_S_StartBackgroundTrack(const char *intro, const char *loop, int fadeupTime);   // empty name stops music
void trap_S_FadeBackgroundTrack(float targetvol, int time, int num);
void trap_S_StopBackgroundTrack(void);
int trap_S_StartStreamingSound(const char *intro, const char *loop, int entnum, int channel, int attenuation);
void trap_S_FadeAllSound(float targetvol, int time, qboolean stopsounds);

void trap_R_LoadWorldMap(const char *mapname);

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t trap_R_RegisterModel(const char *name);             // returns rgb axis if not found
qhandle_t trap_R_RegisterSkin(const char *name);              // returns all white if not found
qhandle_t trap_R_RegisterShader(const char *name);            // returns all white if not found
qhandle_t trap_R_RegisterShaderNoMip(const char *name);       // returns all white if not found

qboolean trap_R_GetSkinModel(qhandle_t skinid, const char *type, char *name);
qhandle_t trap_R_GetShaderFromModel(qhandle_t modelid, int surfnum, int withlightmap);

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void trap_R_ClearScene(void);
void trap_R_AddRefEntityToScene(const refEntity_t *re);

// polys are intended for simple wall marks, not really for doing
// significant construction
void trap_R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts);
void trap_R_AddPolyBufferToScene(polyBuffer_t *pPolyBuffer);

void trap_R_AddPolysToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys);

void trap_R_AddLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags);
void trap_R_AddCoronaToScene(const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible);
void trap_R_RenderScene(const refdef_t *fd);
void trap_R_SetColor(const float *rgba);     // NULL = 1,1,1,1
void trap_R_DrawStretchPic(float x, float y, float w, float h,
                           float s1, float t1, float s2, float t2, qhandle_t hShader);
void trap_R_DrawRotatedPic(float x, float y, float w, float h,
                           float s1, float t1, float s2, float t2, qhandle_t hShader, float angle);
void trap_R_DrawStretchPicGradient(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType);
void trap_R_Add2dPolys(polyVert_t *verts, int numverts, qhandle_t hShader);
void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs);
int trap_R_LerpTag(orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex);
void trap_R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);

// Save out the old render info so we don't kill the LOD system here
void trap_R_SaveViewParms(void);
// Reset the view parameters
void trap_R_RestoreViewParms(void);

// Set fog
void trap_R_SetFog(int fogvar, int var1, int var2, float r, float g, float b, float density);
void trap_R_SetGlobalFog(qboolean restore, int duration, float r, float g, float b, float depthForOpaque);

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void trap_GetGlconfig(glconfig_t *glconfig);

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void trap_GetGameState(gameState_t *gamestate);

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void trap_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime);

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean trap_GetSnapshot(int snapshotNumber, snapshot_t *snapshot);

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean trap_GetServerCommand(int serverCommandNumber);

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int trap_GetCurrentCmdNumber(void);

qboolean trap_GetUserCmd(int cmdNumber, usercmd_t *ucmd);

// used for the weapon/holdable select and zoom
void trap_SetUserCmdValue(int stateValue, int flags, float sensitivityScale, int mpIdentClient);
void trap_SetClientLerpOrigin(float x, float y, float z);

// aids for VM testing
void testPrintInt(char *string, int i);
void testPrintFloat(char *string, float f);

int trap_MemoryRemaining(void);
void trap_R_RegisterFont(const char *fontName, int pointSize, void *font);
qboolean trap_Key_IsDown(int keynum);
int trap_Key_GetCatcher(void);
void trap_Key_SetCatcher(int catcher);
void trap_Key_KeysForBinding(const char *binding, int *key1, int *key2);
int trap_Key_GetKey(const char *binding);
qboolean trap_Key_GetOverstrikeMode(void);
void trap_Key_SetOverstrikeMode(qboolean state);

void trap_UI_Popup(int arg0);

void trap_UI_ClosePopup(const char *arg0);
void trap_Key_GetBindingBuf(int keynum, char *buf, int buflen);
void trap_Key_SetBinding(int keynum, const char *binding);
void trap_Key_KeynumToStringBuf(int keynum, char *buf, int buflen);

void trap_TranslateString(const char *string, char *buf);       //  localization

int trap_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status trap_CIN_StopCinematic(int handle);
e_status trap_CIN_RunCinematic(int handle);
void trap_CIN_DrawCinematic(int handle);
void trap_CIN_SetExtents(int handle, int x, int y, int w, int h);

void trap_SnapVector(float *v);

qboolean trap_GetEntityToken(char *buffer, int bufferSize);
qboolean trap_R_inPVS(const vec3_t p1, const vec3_t p2);
void trap_GetHunkData(int *hunkused, int *hunkexpected);

// binary message channel
qboolean trap_SendMessage(char *buf, int buflen);
messageStatus_t trap_MessageStatus(void);

// dynamic shaders
qboolean trap_R_LoadDynamicShader(const char *shadername, const char *shadertext);
// render to texture
void trap_R_RenderToTexture(int textureid, int x, int y, int w, int h);
int trap_R_GetTextureId(const char *name);
// flush rendering buffer
void trap_R_Finish(void);

bg_playerclass_t *CG_PlayerClassForClientinfo(clientInfo_t *ci, centity_t *cent);

void CG_FitTextToWidth2(char *instr, float scale, float w, int size);
void CG_FitTextToWidth_Ext(char *instr, float scale, float w, int size, fontHelper_t *font);
int CG_TrimLeftPixels(char *instr, float scale, float w, int size);
void CG_FitTextToWidth_SingleLine(char *instr, float scale, float w, int size);

void CG_LocateCampaign(void);
void CG_LocateArena(void);
const char *CG_DescriptionForCampaign(void);
const char *CG_NameForCampaign(void);
void CG_CloseMenus(void);

void CG_LimboMenu_f(void);

extern qboolean ccInitial;

#define CC_FILTER_AXIS          (1 << 0)
#define CC_FILTER_ALLIES        (1 << 1)
#define CC_FILTER_SPAWNS        (1 << 2)
#define CC_FILTER_CMDPOST       (1 << 3)
#define CC_FILTER_HACABINETS    (1 << 4)
#define CC_FILTER_CONSTRUCTIONS (1 << 5)
#define CC_FILTER_DESTRUCTIONS  (1 << 6)
#define CC_FILTER_LANDMINES     (1 << 7)
#define CC_FILTER_OBJECTIVES    (1 << 8)
// #define CC_FILTER_BUDDIES       (1 << 9)
// #define CC_FILTER_REQUESTS      (1 << 10)

/**
 * @struct rankicon_t
 * @brief
 */
typedef struct
{
	qhandle_t shader;
	const char *iconname;
	int width;
	int height;
} rankicon_t;

extern rankicon_t rankicons[NUM_EXPERIENCE_LEVELS][2][2];

// merged the common UI elements
#define UI_CAMPAIGN_BRIEFING 0
#define UI_COMMAND_MAP 1
#define UI_SQUAD_SELECT 2

qboolean CG_UICommonClick(void);
void CG_DrawUISelectedSoldier(void);
void CG_UICurrentSquadSetup(void);
void CG_CampaignBriefingSetup(void);

// Fireteam stuff
fireteamData_t *CG_IsOnFireteam(int clientNum);
fireteamData_t *CG_IsOnSameFireteam(int clientNum, int clientNum2);
fireteamData_t *CG_IsFireTeamLeader(int clientNum);

//clientInfo_t *CG_ClientInfoForPosition(int pos, int max);
//fireteamData_t *CG_FireTeamForPosition(int pos, int max);
//clientInfo_t *CG_FireTeamPlayerForPosition(int pos, int max);

void CG_SortClientFireteam(void);

void CG_DrawFireTeamOverlay(rectDef_t *rect);
clientInfo_t *CG_SortedFireTeamPlayerForPosition(int pos);
qboolean CG_FireteamHasClass(int classnum, qboolean selectedonly);
const char *CG_BuildSelectedFirteamString(void);

#define Pri(x) CG_Printf("[cgnotify]%s", CG_LocalizeServerCommand(x))
#define CPri(x) CG_CenterPrint(CG_LocalizeServerCommand(x), 400, cg_fontScaleCP.value)

#ifdef FEATURE_MULTIVIEW
// cg_multiview.c
void CG_mvDelete_f(void);
void CG_mvHideView_f(void);
void CG_mvNew_f(void);
void CG_mvShowView_f(void);
void CG_mvSwapViews_f(void);
void CG_mvToggleAll_f(void);
void CG_mvToggleView_f(void);

cg_window_t *CG_mvClientLocate(int pID);
void CG_mvCreate(int pID);
cg_window_t *CG_mvCurrent(void);
void CG_mvDraw(cg_window_t *sw);
cg_window_t *CG_mvFindNonMainview(void);
void CG_mvFree(int pID);
void CG_mvMainviewSwap(cg_window_t *av);
qboolean CG_mvMergedClientLocate(int pID);
void CG_mvOverlayDisplay(void);
void CG_mvOverlayUpdate(void);
void CG_mvOverlayClientUpdate(int pID, int index);
void CG_mvProcessClientList(void);
void CG_mvTransitionPlayerState(playerState_t *ps);
void CG_mvUpdateClientInfo(int pID);
void CG_mvWindowOverlay(int pID, float b_x, float b_y, float b_w, float b_h, float s, int wState, qboolean fSelected);
void CG_mvZoomBinoc(float x, float y, float w, float h);
void CG_mvZoomSniper(float x, float y, float w, float h);
void CG_mv_KeyHandling(int _key, qboolean down);
#endif

// cg_window.c
qboolean CG_addString(cg_window_t *w, const char *buf);
void CG_createStatsWindow(void);
void CG_createTopShotsWindow(void);
void CG_createWstatsMsgWindow(void);
void CG_createWtopshotsMsgWindow(void);
void CG_createMOTDWindow(void);
#ifdef FEATURE_MULTIVIEW
void CG_cursorUpdate(void);
#endif
void CG_initStrings(void);
void CG_printWindow(const char *str);
void CG_removeStrings(cg_window_t *w);
cg_window_t *CG_windowAlloc(int fx, int startupLength);
void CG_windowDraw(void);
void CG_windowFree(cg_window_t *w);
void CG_windowInit(void);
void CG_windowNormalizeOnText(cg_window_t *w);

void CG_SetupCabinets(void);

extern displayContextDef_t cgDC;
void CG_ParseSkyBox(void);
void CG_ParseTagConnect(int tagNum);
void CG_ParseTagConnects(void);

// cg_ents.c
void CG_AttachBitsToTank(centity_t *tank, refEntity_t *mg42base, refEntity_t *mg42upper, refEntity_t *mg42gun, refEntity_t *player, refEntity_t *flash, vec_t *playerangles, const char *tagName, qboolean browning);

// cg_polybus.c
polyBuffer_t *CG_PB_FindFreePolyBuffer(qhandle_t shader, int numVerts, int numIndicies);
void CG_PB_ClearPolyBuffers(void);
void CG_PB_RenderPolyBuffers(void);

// cg_limbopanel.c
void CG_LimboPanel_KeyHandling(int key, qboolean down);
int CG_LimboPanel_GetMaxObjectives(void);

qboolean CG_LimboPanel_WeaponLights_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_WeaponPanel_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_WeaponPanel_KeyUp(panel_button_t *button, int key);
qboolean CG_LimboPanel_ObjectiveText_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_TeamButton_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_ClassButton_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_OkButton_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_PlusButton_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_MinusButton_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_CancelButton_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_Filter_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_BriefingButton_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_SpawnPointButton_KeyDown(panel_button_t *button, int key);

void CG_LimboPanel_SpawnPointButton_Draw(panel_button_t *button);
void CG_LimboPanel_BriefingButton_Draw(panel_button_t *button);
void CG_LimboPanel_ClassBar_Draw(panel_button_t *button);
void CG_LimboPanel_Filter_Draw(panel_button_t *button);
void CG_LimboPanel_RenderSkillIcon(panel_button_t *button);
void CG_LimboPanel_RenderTeamButton(panel_button_t *button);
void CG_LimboPanel_RenderClassButton(panel_button_t *button);
void CG_LimboPanel_RenderObjectiveText(panel_button_t *button);
void CG_LimboPanel_RenderCommandMap(panel_button_t *button);
void CG_LimboPanel_RenderObjectiveBack(panel_button_t *button);
void CG_LimboPanel_RenderLight(panel_button_t *button);
void CG_LimboPanel_WeaponLights(panel_button_t *button);
void CG_LimboPanel_RenderHead(panel_button_t *button);
void CG_LimboPanel_WeaponPanel(panel_button_t *button);
void CG_LimboPanel_Border_Draw(panel_button_t *button);
void CG_LimboPanel_RenderMedal(panel_button_t *button);
void CG_LimboPanel_RenderCounter(panel_button_t *button);
void CG_LimboPanelRenderText_NoLMS(panel_button_t *button);
void CG_LimboPanelRenderText_SkillsText(panel_button_t *button);
#ifdef FEATURE_PRESTIGE
void CG_LimboPanel_RenderPrestige(panel_button_t *button);
void CG_LimboPanel_RenderPrestigeIcon(panel_button_t *button);
void CG_LimboPanel_Prestige_Draw(panel_button_t *button);
#endif

void CG_LimboPanel_NameEditFinish(panel_button_t *button);

void CG_LimboPanel_Setup(void);
void CG_LimboPanel_Init(void);

void CG_LimboPanel_RequestObjective(void);
void CG_LimboPanel_RequestWeaponStats(void);
qboolean CG_LimboPanel_Draw(void);
int CG_LimboPanel_GetSpawnPoint(void);
team_t CG_LimboPanel_GetTeam(void);
team_t CG_LimboPanel_GetRealTeam(void);
bg_character_t *CG_LimboPanel_GetCharacter(void);
int CG_LimboPanel_GetClass(void);
int CG_LimboPanel_ClassCount(team_t checkTeam, int classIndex);
int CG_LimboPanel_MaxCount(int playerCount, const char *variableString);
int CG_LimboPanel_TeamCount(int weap);
int CG_LimboPanel_WeaponCount(int number);
weapon_t CG_LimboPanel_GetSelectedWeapon(int slot);
void CG_LimboPanel_SetSelectedWeaponNum(int slot, weapon_t weapon);
bg_playerclass_t *CG_LimboPanel_GetPlayerClass(void);
weapon_t CG_LimboPanel_GetWeaponForNumber(int number, int slot, qboolean ignoreDisabled);
qboolean CG_LimboPanel_IsValidSelectedWeapon(int slot);
void CG_LimboPanel_SetDefaultWeapon(int slot);
qboolean CG_LimboPanel_RealWeaponIsDisabled(weapon_t weapon);
qboolean CG_LimboPanel_ClassIsDisabled(team_t selectedTeam, int classIndex);
qboolean CG_LimboPanel_TeamIsDisabled(team_t checkTeam);
int CG_LimboPanel_FindFreeClass(team_t checkTeam);
int CG_LimboPanel_GetWeaponNumberForPos(int pos);

/**
 * @struct mapScissor_s
 * @typedef mapScissor_t
 * @brief A scissored map always has the player in the center
 * @see cg_commandmap.c
 */
typedef struct mapScissor_s
{
	qboolean circular;  ///< if qfalse, rect
	float zoomFactor;
	vec2_t tl;
	vec2_t br;
} mapScissor_t;

int CG_CurLayerForZ(int z);
void CG_DrawMap(float x, float y, float w, float h, int mEntFilter, mapScissor_t *scissor, qboolean interactive, float alpha, qboolean borderblend);
int CG_DrawSpawnPointInfo(float px, float py, float pw, float ph, qboolean draw, mapScissor_t *scissor, int expand);
void CG_DrawMortarMarker(float px, float py, float pw, float ph, qboolean draw, mapScissor_t *scissor, int expand);
void CG_CommandMap_SetHighlightText(const char *text, float x, float y);
void CG_CommandMap_DrawHighlightText(void);
qboolean CG_CommandCentreSpawnPointClick(void);

void CG_DrawAutoMapNew(float x, float y, float w, float h);
void CG_DrawMapNew(float x, float y, float w, float h, int mEntFilter, mapScissor_t *scissor, qboolean interactive, float alpha, qboolean borderblend);
int CG_DrawSpawnPointInfoNew(float px, float py, float pw, float ph, qboolean draw, mapScissor_t *scissor, int expand);
void CG_DrawMortarMarkerNew(float px, float py, float pw, float ph, qboolean draw, mapScissor_t *scissor, int expand);

#define LIMBO_3D_X  287 //% 280
#define LIMBO_3D_Y  382
#define LIMBO_3D_W  128
#define LIMBO_3D_H  96  //% 94

#define CC_2D_X 64
#define CC_2D_Y 23
#define CC_2D_W 352
#define CC_2D_H 352

void CG_DrawPlayerHead(rectDef_t *rect, bg_character_t *character, bg_character_t *headcharacter, float yaw, float pitch, qboolean drawHat, hudHeadAnimNumber_t animation, qhandle_t painSkin, int rank, qboolean spectator, int team);

// cg_popupmessages.c
void CG_InitPM(void);
void CG_InitPMGraphics(void);
void CG_UpdatePMLists(void);
void CG_AddPMItem(popupMessageType_t type, const char *message, const char *message2, qhandle_t shader, qhandle_t weaponShader, int scaleShader, vec3_t color);
void CG_AddPMItemBig(popupMessageBigType_t type, const char *message, qhandle_t shader);
void CG_DrawPMItems(rectDef_t rect, int style);
void CG_DrawPMItemsBig(int style);
const char *CG_GetPMItemText(centity_t *cent);
void CG_PlayPMItemSound(centity_t *cent);
qhandle_t CG_GetPMItemIcon(centity_t *cent);

// cg_debriefing.c
clientInfo_t *CG_Debriefing_GetSelectedClientInfo(void);
void CG_Debrieing_SetSelectedClient(int clientNum);

qboolean CG_Debriefing_Draw(void);
void CG_ChatPanel_Setup(void);

void CG_Debriefing_ChatEditFinish(panel_button_t *button);
void CG_Debriefing_VoteButton_Draw(panel_button_t *button);
void CG_Debriefing_VoteNowButton_Draw(panel_button_t *button);
void CG_Debriefing_NextButton_Draw(panel_button_t *button);
void CG_Debriefing_ChatButton_Draw(panel_button_t *button);
void CG_Debriefing_ReadyButton_Draw(panel_button_t *button);
#ifdef FEATURE_PRESTIGE
void CG_Debriefing_PrestigeButton_Draw(panel_button_t *button);
#endif
qboolean CG_Debriefing_ChatButton_KeyDown(panel_button_t *button, int key);
qboolean CG_Debriefing_ReadyButton_KeyDown(panel_button_t *button, int key);
qboolean CG_Debriefing_QCButton_KeyDown(panel_button_t *button, int key);
qboolean CG_Debriefing_VoteButton_KeyDown(panel_button_t *button, int key);
qboolean CG_Debriefing_NextButton_KeyDown(panel_button_t *button, int key);
#ifdef FEATURE_PRESTIGE
qboolean CG_Debriefing_PrestigeButton_KeyDown(panel_button_t *button, int key);
#endif

void CG_PanelButtonsRender_Button_Ext(rectDef_t *r, const char *text);

void CG_Debriefing_PlayerName_Draw(panel_button_t *button);
void CG_Debriefing_PlayerRank_Draw(panel_button_t *button);
void CG_Debriefing_PlayerMedals_Draw(panel_button_t *button);
void CG_Debriefing_PlayerTime_Draw(panel_button_t *button);
void CG_Debriefing_PlayerXP_Draw(panel_button_t *button);
#ifdef FEATURE_RATING
void CG_Debriefing_PlayerSR_Draw(panel_button_t *button);
#endif
void CG_Debriefing_PlayerACC_Draw(panel_button_t *button);
void CG_Debriefing_PlayerHS_Draw(panel_button_t *button);
void CG_Debriefing_PlayerSkills_Draw(panel_button_t *button);
#ifdef FEATURE_PRESTIGE
void CG_Debriefing_PlayerPrestige_Draw(panel_button_t *button);
void CG_Debriefing_PlayerPrestige_Note(panel_button_t *button);
#endif
void CG_Debriefing_PlayerHitRegions_Draw(panel_button_t *button);

void CG_DebriefingPlayerWeaponStats_Draw(panel_button_t *button);

void CG_DebriefingXPHeader_Draw(panel_button_t *button);

void CG_DebriefingTitle_Draw(panel_button_t *button);
void CG_DebriefingPlayerList_Draw(panel_button_t *button);
qboolean CG_DebriefingPlayerList_KeyDown(panel_button_t *button, int key);

void CG_Debriefing_ChatEdit_Draw(panel_button_t *button);
void CG_Debriefing_ChatBox_Draw(panel_button_t *button);
void CG_Debriefing_Scrollbar_Draw(panel_button_t *button);
qboolean CG_Debriefing_Scrollbar_KeyDown(panel_button_t *button, int key);
qboolean CG_Debriefing_Scrollbar_KeyUp(panel_button_t *button, int key);
float CG_Debriefing_CalcCampaignProgress(void);

const char *CG_Debriefing_RankNameForClientInfo(clientInfo_t *ci);
const char *CG_Debriefing_FullRankNameForClientInfo(clientInfo_t *ci);
void CG_Debriefing_Startup(void);
void CG_Debriefing_Shutdown(void);
void CG_Debriefing_MouseEvent(int x, int y);

void CG_Debriefing_ParseWeaponAccuracies(void);
void CG_Debriefing_ParseWeaponStats(void);
void CG_Debriefing_ParsePlayerKillsDeaths(void);
void CG_Debriefing_ParsePlayerTime(void);
void CG_Debriefing_ParseAwards(void);
void CG_Debriefing_ParseSkillRating(void);
#ifdef FEATURE_PRESTIGE
void CG_Debriefing_ParsePrestige(void);
#endif

void CG_TeamDebriefingTeamSkillXP_Draw(panel_button_t *button);

const char *CG_PickupItemText(int itemNum);

void CG_LoadPanel_DrawPin(const char *text, float px, float py, float sx, float sy, qhandle_t shader, float pinsize, float backheight);
void CG_LoadPanel_RenderCampaignPins(panel_button_t *button);
void CG_LoadPanel_RenderMissionDescriptionText(panel_button_t *button);
void CG_LoadPanel_RenderCampaignTypeText(panel_button_t *button);
void CG_LoadPanel_RenderCampaignNameText(panel_button_t *button);
void CG_LoadPanel_RenderLoadingBar(panel_button_t *button);
void CG_LoadPanel_LoadingBarText(panel_button_t *button);
void CG_LoadPanel_KeyHandling(int key, qboolean down);
void CG_DrawConnectScreen(qboolean interactive, qboolean forcerefresh);

qboolean CG_Debriefing_Maps_KeyDown(panel_button_t *button, int key);
void CG_Debriefing_TeamSkillHeaders_Draw(panel_button_t *button);
void CG_Debriefing_TeamSkillXP_Draw(panel_button_t *button);
void CG_Debriefing_MissionTitle_Draw(panel_button_t *button);
void CG_Debriefing_Mission_Draw(panel_button_t *button);
void CG_Debriefing_Maps_Draw(panel_button_t *button);
void CG_Debriefing_Awards_Draw(panel_button_t *button);
void CG_PanelButtonsRender_Window(panel_button_t *button);
void CG_PanelButtonsRender_Button(panel_button_t *button);

team_t CG_Debriefing_FindWinningTeamForMap(void);

int CG_CalcViewValues(void);
void CG_HudHeadAnimation(bg_character_t *ch, lerpFrame_t *lf, int *oldframe, int *frame, float *backlerp, hudHeadAnimNumber_t animation);

// cg_fireteams.c
void CG_Fireteams_KeyHandling(int key, qboolean down);
qboolean CG_FireteamCheckExecKey(int key, qboolean doaction);
void CG_Fireteams_Draw(void);
void CG_Fireteams_Setup(void);

void CG_Fireteams_MenuText_Draw(panel_button_t *button);
void CG_Fireteams_MenuTitleText_Draw(panel_button_t *button);

// cg_spawnpoints.c
void CG_Spawnpoints_KeyHandling(int key, qboolean down);
qboolean CG_SpawnpointsCheckExecKey(int key, qboolean doaction);
void CG_Spawnpoints_Draw(void);
void CG_Spawnpoints_Setup(void);

void CG_Spawnpoints_MenuText_Draw(panel_button_t *button);
void CG_Spawnpoints_MenuTitleText_Draw(panel_button_t *button);

// hitsounds flags
/**
 * @enum hitsooundFlags
 * @brief
 */
typedef enum
{
	HITSOUNDS_ON         = BIT(0),
	HITSOUNDS_NOBODYSHOT = BIT(1),
	HITSOUNDS_NOHEADSHOT = BIT(2),
	HITSOUNDS_NOTEAMSHOT = BIT(3),
} hitsooundFlags;

// Safe screenwidth and screenheight defines
#define SCREEN_WIDTH_SAFE Ccg_WideX(SCREEN_WIDTH)
#define SCREEN_HEIGHT_SAFE SCREEN_HEIGHT

#define IS_MOUNTED_TANK_BROWNING(entNum) (cg_entities[cg_entities[cg_entities[entNum].tagParent].tankparent].currentState.density & 8)

extern qboolean resetmaxspeed; // CG_DrawSpeed

/* HUD exports */

typedef struct hudComponent_s
{
	rectDef_t location;
	int visible;
	int style;
} hudComponent_t;

typedef struct hudStructure_s
{
	int hudnumber;
	hudComponent_t compas;
	hudComponent_t staminabar;
	hudComponent_t breathbar;
	hudComponent_t healthbar;
	hudComponent_t weaponchargebar;
	hudComponent_t healthtext;
	hudComponent_t xptext;
	hudComponent_t ranktext;
	hudComponent_t statsdisplay;
	hudComponent_t weaponicon;
	hudComponent_t weaponammo;
	hudComponent_t fireteam;
	hudComponent_t popupmessages;
	hudComponent_t powerups;
	hudComponent_t hudhead;

	hudComponent_t cursorhint;
	hudComponent_t weaponstability;
	hudComponent_t livesleft;

	hudComponent_t roundtimer;
	hudComponent_t reinforcement;
	hudComponent_t spawntimer;
	hudComponent_t localtime;

	hudComponent_t votetext;
	hudComponent_t spectatortext;
	hudComponent_t limbotext;
	hudComponent_t followtext;

} hudStucture_t;

hudStucture_t *CG_GetActiveHUD();

#endif // #ifndef INCLUDE_CG_LOCAL_H
