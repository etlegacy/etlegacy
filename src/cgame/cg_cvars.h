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

#ifndef INCLUDE_CG_CVARS_H
#define INCLUDE_CG_CVARS_H

#include "../qcommon/q_shared.h"

extern vmCvar_t cg_centertime;
extern vmCvar_t cg_bobbing;

extern vmCvar_t cg_swingSpeed;
extern vmCvar_t cg_shadows;
extern vmCvar_t cg_gibs;
extern vmCvar_t cg_draw2D;
extern vmCvar_t cg_drawFPS;
extern vmCvar_t cg_drawCrosshair;
extern vmCvar_t cg_drawCrosshairFade;
extern vmCvar_t cg_drawCrosshairPickups;
extern vmCvar_t cg_drawSpectatorNames;
extern vmCvar_t cg_drawHintFade;
extern vmCvar_t cg_useWeapsForZoom;
extern vmCvar_t cg_weaponCycleDelay;
extern vmCvar_t cg_cycleAllWeaps;
extern vmCvar_t cg_drawStatus;
extern vmCvar_t cg_animSpeed;
extern vmCvar_t cg_debugAnim;
extern vmCvar_t cg_debugPosition;
extern vmCvar_t cg_debugEvents;
extern vmCvar_t cg_railTrailTime;
extern vmCvar_t cg_errorDecay;
extern vmCvar_t cg_nopredict;
extern vmCvar_t cg_noPlayerAnims;
extern vmCvar_t cg_showmiss;
extern vmCvar_t cg_markTime;
extern vmCvar_t cg_bloodPuff;
extern vmCvar_t cg_brassTime;
extern vmCvar_t cg_gun_frame;
extern vmCvar_t cg_gunFovOffset;
extern vmCvar_t cg_gunReviveFadeIn;
extern vmCvar_t cg_gun_x;
extern vmCvar_t cg_gun_y;
extern vmCvar_t cg_gun_z;
extern vmCvar_t cg_drawGun;
extern vmCvar_t cg_weapAnims;
extern vmCvar_t cg_weapBankCollisions;
extern vmCvar_t cg_weapSwitchNoAmmoSounds;
extern vmCvar_t cg_letterbox;
extern vmCvar_t cg_tracerChance;
extern vmCvar_t cg_tracerWidth;
extern vmCvar_t cg_tracerLength;
extern vmCvar_t cg_tracerSpeed;
extern vmCvar_t cg_autoswitch;
extern vmCvar_t cg_fov;
extern vmCvar_t cg_muzzleFlash;
extern vmCvar_t cg_muzzleFlashDlight;
extern vmCvar_t cg_muzzleFlashOld;
extern vmCvar_t cg_drawEnvAwareness;
extern vmCvar_t cg_drawEnvAwarenessScale;
extern vmCvar_t cg_drawEnvAwarenessIconSize;
extern vmCvar_t cg_dynamicIcons;
extern vmCvar_t cg_dynamicIconsDistance;
extern vmCvar_t cg_dynamicIconsSize;
extern vmCvar_t cg_dynamicIconsMaxScale;
extern vmCvar_t cg_dynamicIconsMinScale;

extern vmCvar_t cg_zoomDefaultSniper;

extern vmCvar_t cg_zoomStepSniper;
extern vmCvar_t cg_thirdPersonRange;
extern vmCvar_t cg_thirdPersonAngle;
extern vmCvar_t cg_thirdPerson;
extern vmCvar_t cg_scopedSensitivityScaler;
#ifdef ALLOW_GSYNC
extern vmCvar_t cg_synchronousClients;
#endif // ALLOW_GSYNC
extern vmCvar_t cg_teamChatTime;
extern vmCvar_t cg_teamChatMention;
extern vmCvar_t cg_stats;
extern vmCvar_t cg_coronafardist;
extern vmCvar_t cg_coronas;
extern vmCvar_t cg_buildScript;
extern vmCvar_t cg_paused;
extern vmCvar_t cg_blood;
extern vmCvar_t cg_predictItems;
extern vmCvar_t cg_teamChatsOnly;
extern vmCvar_t cg_teamVoiceChatsOnly;
extern vmCvar_t cg_voiceChats;
extern vmCvar_t cg_voiceText;
extern vmCvar_t cg_autoactivate;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;

extern vmCvar_t cg_timescale;

extern vmCvar_t cg_spritesFollowHeads;
extern vmCvar_t cg_voiceSpriteTime;

extern vmCvar_t cg_gameType;
extern vmCvar_t cg_bloodTime;
extern vmCvar_t cg_skybox;

extern vmCvar_t cg_redlimbotime;
extern vmCvar_t cg_bluelimbotime;

extern vmCvar_t cg_limboClassClickConfirm;

extern vmCvar_t cg_movespeed;

extern vmCvar_t cg_drawNotifyText;
extern vmCvar_t cg_quickMessageAlt;

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
extern vmCvar_t cg_bloodForcePuffsForDamage;
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

extern vmCvar_t cg_debugSkills;

// some optimization cvars
extern vmCvar_t cg_instanttapout;

// demo recording cvars
extern vmCvar_t cl_demorecording;
extern vmCvar_t cl_demofilename;
extern vmCvar_t cl_demooffset;
extern vmCvar_t cl_waverecording;
extern vmCvar_t cl_wavefilename;
extern vmCvar_t cl_waveoffset;

extern vmCvar_t cg_announcer;
extern vmCvar_t cg_hitSounds;
extern vmCvar_t cg_locations;
extern vmCvar_t cg_locationMaxChars;

extern vmCvar_t cg_spawnTimer_period;
extern vmCvar_t cg_spawnTimer_set;

extern vmCvar_t cg_logFile;

extern vmCvar_t cg_countryflags; ///< GeoIP

extern vmCvar_t cg_altHud;
extern vmCvar_t cg_shoutcasterHud;
extern vmCvar_t cg_tracers;
extern vmCvar_t cg_fireteamNameMaxChars;
extern vmCvar_t cg_fireteamNameAlign;
extern vmCvar_t cg_fireteamSprites;
extern vmCvar_t cg_fireteamSpritesColor;
extern vmCvar_t cg_fireteamSpritesColorSelected;

extern vmCvar_t cg_simpleItems;
extern vmCvar_t cg_simpleItemsScale;

extern vmCvar_t cg_weapaltReloads;
extern vmCvar_t cg_weapaltSwitches;
extern vmCvar_t cg_weapaltMgAutoProne;

extern vmCvar_t cg_sharetimerText;

extern vmCvar_t cg_automapZoom;
extern vmCvar_t cg_autoCmd;

extern vmCvar_t cg_popupFadeTime;
extern vmCvar_t cg_popupStayTime;
extern vmCvar_t cg_popupTime;

extern vmCvar_t cg_popupXPGainFadeTime;
extern vmCvar_t cg_popupXPGainStayTime;
extern vmCvar_t cg_popupXPGainTime;

extern vmCvar_t cg_fontScaleSP;

// unlagged optimized prediction
extern vmCvar_t cg_optimizePrediction;
extern vmCvar_t cg_debugPlayerHitboxes;
extern vmCvar_t cg_debugBullets;

// scoreboard
extern vmCvar_t cg_scoreboard;

extern vmCvar_t cg_quickchat;

extern vmCvar_t cg_drawUnit;

extern vmCvar_t cg_visualEffects;  ///< turn invisible (0) / visible (1) visual effect (e.g. smoke, debris, ...)
extern vmCvar_t cg_bannerTime;

extern vmCvar_t cg_shoutcastTeamNameRed;
extern vmCvar_t cg_shoutcastTeamNameBlue;
extern vmCvar_t cg_shoutcastDrawHealth;
extern vmCvar_t cg_shoutcastGrenadeTrail;

extern vmCvar_t cg_activateLean;

extern vmCvar_t cg_drawBreathPuffs;
extern vmCvar_t cg_drawAirstrikePlanes;

extern vmCvar_t cg_customFont1;
extern vmCvar_t cg_customFont2;

extern vmCvar_t cg_drawSpawnpoints;
extern vmCvar_t cg_crosshairSVG;
extern vmCvar_t cg_crosshairSize;
extern vmCvar_t cg_crosshairAlpha;
extern vmCvar_t cg_crosshairColor;
extern vmCvar_t cg_crosshairAlphaAlt;
extern vmCvar_t cg_crosshairColorAlt;
extern vmCvar_t cg_crosshairPulse;
extern vmCvar_t cg_crosshairHealth;
extern vmCvar_t cg_crosshairX;
extern vmCvar_t cg_crosshairY;
extern vmCvar_t cg_crosshairScaleX;
extern vmCvar_t cg_crosshairScaleY;

extern vmCvar_t cg_customCrosshair;
extern vmCvar_t cg_customCrosshairDotWidth;
extern vmCvar_t cg_customCrosshairDotColor;
extern vmCvar_t cg_customCrosshairDotOutlineRounded;
extern vmCvar_t cg_customCrosshairDotOutlineColor;
extern vmCvar_t cg_customCrosshairDotOutlineWidth;
extern vmCvar_t cg_customCrosshairCrossWidth;
extern vmCvar_t cg_customCrosshairCrossLength;
extern vmCvar_t cg_customCrosshairCrossGap;
extern vmCvar_t cg_customCrosshairCrossSpreadDistance;
extern vmCvar_t cg_customCrosshairCrossSpreadOTGCoef;
extern vmCvar_t cg_customCrosshairCrossColor;
extern vmCvar_t cg_customCrosshairCrossOutlineRounded;
extern vmCvar_t cg_customCrosshairCrossOutlineColor;
extern vmCvar_t cg_customCrosshairCrossOutlineWidth;
extern vmCvar_t cg_customCrosshairHealth;

extern vmCvar_t cg_scopeReticleStyle;
extern vmCvar_t cg_scopeReticleColor;
extern vmCvar_t cg_scopeReticleDotColor;
extern vmCvar_t cg_scopeReticleLineThickness;
extern vmCvar_t cg_scopeReticleDotThickness;

extern vmCvar_t cg_commandMapTime;

void CG_RegisterCvars(void);

#endif  // #ifndef INCLUDE_CG_CVARS_H
