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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "cl_cvars.h"

// CVARS {{{1

#ifdef USE_RENDERER_DLOPEN
cvar_t *cl_renderer;
#endif

cvar_t *cl_wavefilerecord;
cvar_t *cl_nodelta;
cvar_t *cl_debugMove;

cvar_t *cl_noprint;

cvar_t *rcon_client_password;
cvar_t *rconAddress;

cvar_t *cl_timeout;
cvar_t *cl_maxpackets;
cvar_t *cl_packetdup;
cvar_t *cl_timeNudge;
cvar_t *cl_extrapolationMargin;
cvar_t *cl_showTimeDelta;
cvar_t *cl_freezeDemo;

cvar_t *cl_shownet = NULL;      // This is referenced in msg.c and we need to make sure it is NULL
cvar_t *cl_shownuments;
cvar_t *cl_showSend;
cvar_t *cl_showServerCommands;
cvar_t *cl_timedemo;
cvar_t *cl_avidemo;
cvar_t *cl_forceavidemo;
cvar_t *cl_avidemotype;
cvar_t *cl_aviMotionJpeg;
cvar_t *cl_aviFrameRate;
cvar_t *cl_aviPipeFormat;
cvar_t *cl_aviPipeExtension;

cvar_t *cl_freelook;
cvar_t *cl_sensitivity;

cvar_t *cl_mouseAccel;
cvar_t *cl_showMouseRate;

cvar_t *m_pitch;
cvar_t *m_yaw;
cvar_t *m_forward;
cvar_t *m_side;
cvar_t *m_filter;

cvar_t *j_pitch;
cvar_t *j_yaw;
cvar_t *j_forward;
cvar_t *j_side;
cvar_t *j_up;
cvar_t *j_pitch_axis;
cvar_t *j_yaw_axis;
cvar_t *j_forward_axis;
cvar_t *j_side_axis;
cvar_t *j_up_axis;

cvar_t *cl_activeAction;

cvar_t *cl_autorecord;

cvar_t *cl_allowDownload;
cvar_t *cl_wwwDownload;
cvar_t *cl_conXOffset;

cvar_t *cl_serverStatusResendTime;
cvar_t *cl_missionStats;

cvar_t *cl_profile;
cvar_t *cl_defaultProfile;

cvar_t *cl_demorecording;
cvar_t *cl_demofilename;
cvar_t *cl_demooffset;

cvar_t *cl_waverecording;
cvar_t *cl_wavefilename;
cvar_t *cl_waveoffset;

cvar_t *cl_packetloss;
cvar_t *cl_packetdelay;

cvar_t *cl_consoleKeys;

cvar_t *cl_interpolation;

// Functions {{{1
void CL_InitCvars()
{
	// register our variables
	cl_noprint = Cvar_Get("cl_noprint", "0", 0);

	cl_timeout = Cvar_Get("cl_timeout", "200", 0);

	cl_wavefilerecord = Cvar_Get("cl_wavefilerecord", "0", CVAR_TEMP);

	cl_extrapolationMargin = Cvar_Get("cl_extrapolationMargin", "1", CVAR_ARCHIVE);
	Cvar_CheckRange(cl_extrapolationMargin, 0, 10, qtrue);

	cl_timeNudge          = Cvar_Get("cl_timeNudge", "0", CVAR_TEMP);
	cl_shownet            = Cvar_Get("cl_shownet", "0", CVAR_TEMP);
	cl_shownuments        = Cvar_Get("cl_shownuments", "0", CVAR_TEMP);
	cl_showServerCommands = Cvar_Get("cl_showServerCommands", "0", 0);
	cl_showSend           = Cvar_Get("cl_showSend", "0", CVAR_TEMP);
	cl_showTimeDelta      = Cvar_Get("cl_showTimeDelta", "0", CVAR_TEMP);
	cl_freezeDemo         = Cvar_Get("cl_freezeDemo", "0", CVAR_ROM);
	rcon_client_password  = Cvar_Get("rconPassword", "", CVAR_TEMP);
	cl_activeAction       = Cvar_Get("activeAction", "", CVAR_TEMP);
	cl_autorecord         = Cvar_Get("cl_autorecord", "0", CVAR_TEMP);

	cl_timedemo      = Cvar_Get("timedemo", "0", 0);
	cl_avidemo       = Cvar_Get("cl_avidemo", "0", CVAR_TEMP);
	cl_forceavidemo  = Cvar_Get("cl_forceavidemo", "0", CVAR_TEMP);
	cl_avidemotype   = Cvar_Get("cl_avidemotype", "0", CVAR_ARCHIVE);
	cl_aviMotionJpeg = Cvar_Get("cl_avimotionjpeg", "0", CVAR_TEMP);

	cl_aviFrameRate = Cvar_GetAndDescribe("cl_aviFrameRate", "25", CVAR_ARCHIVE, "Framerate to use for video recording with video and video-pipe");
	Cvar_CheckRange(cl_aviFrameRate, 1, 1000, qtrue);

	cl_aviPipeFormat = Cvar_GetAndDescribe("cl_aviPipeFormat",
	                                       "-preset medium -crf 23 -vcodec libx264 -flags +cgop -pix_fmt yuvj420p -bf 2 -codec:a aac -strict -2 -b:a 160k -movflags faststart",
	                                       CVAR_ARCHIVE, "ffmpeg command line passed to the encoder when using video-pipe");
	cl_aviPipeExtension = Cvar_GetAndDescribe("cl_aviPipeExtension", "mp4", CVAR_ARCHIVE, "Extension to use for video files when using video-pipe");

	rconAddress = Cvar_Get("rconAddress", "", 0);

	cl_yawspeed      = Cvar_Get("cl_yawspeed", "140", CVAR_ARCHIVE_ND);
	cl_pitchspeed    = Cvar_Get("cl_pitchspeed", "140", CVAR_ARCHIVE_ND);
	cl_anglespeedkey = Cvar_Get("cl_anglespeedkey", "1.5", 0);

	cl_maxpackets = Cvar_Get("cl_maxpackets", "125", CVAR_ARCHIVE);
	cl_packetdup  = Cvar_Get("cl_packetdup", "1", CVAR_ARCHIVE_ND);

	cl_run         = Cvar_Get("cl_run", "1", CVAR_ARCHIVE_ND);
	cl_sensitivity = Cvar_Get("sensitivity", "5", CVAR_ARCHIVE);
	cl_mouseAccel  = Cvar_Get("cl_mouseAccel", "0", CVAR_ARCHIVE_ND);
	cl_freelook    = Cvar_Get("cl_freelook", "1", CVAR_ARCHIVE_ND);

	cl_showMouseRate = Cvar_Get("cl_showmouserate", "0", 0);

	cl_allowDownload = Cvar_Get("cl_allowDownload", "1", CVAR_ARCHIVE_ND);
	cl_wwwDownload   = Cvar_Get("cl_wwwDownload", "1", CVAR_USERINFO | CVAR_ARCHIVE_ND);

	cl_profile        = Cvar_Get("cl_profile", "", CVAR_ROM);
	cl_defaultProfile = Cvar_Get("cl_defaultProfile", "", CVAR_ROM);

	// init autoswitch so the ui will have it correctly even
	// if the cgame hasn't been started
	// disabled autoswitch by default
	Cvar_Get("cg_autoswitch", "0", CVAR_ARCHIVE);

	cl_conXOffset = Cvar_Get("cl_conXOffset", "0", 0);

	cl_serverStatusResendTime = Cvar_Get("cl_serverStatusResendTime", "750", 0);

	cl_recoilPitch = Cvar_Get("cg_recoilPitch", "0", CVAR_ROM);

	cl_bypassMouseInput = Cvar_Get("cl_bypassMouseInput", "0", 0);    //CVAR_ROM );

	cl_doubletapdelay = Cvar_Get("cl_doubletapdelay", "0", CVAR_ARCHIVE_ND);    // double tap

	m_pitch   = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE_ND);
	m_yaw     = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE_ND);
	m_forward = Cvar_Get("m_forward", "0.25", CVAR_ARCHIVE_ND);
	m_side    = Cvar_Get("m_side", "0.25", CVAR_ARCHIVE_ND);
	m_filter  = Cvar_Get("m_filter", "0", CVAR_ARCHIVE_ND);

	j_pitch   = Cvar_Get("j_pitch", "0.022", CVAR_ARCHIVE_ND);
	j_yaw     = Cvar_Get("j_yaw", "-0.022", CVAR_ARCHIVE_ND);
	j_forward = Cvar_Get("j_forward", "-0.25", CVAR_ARCHIVE_ND);
	j_side    = Cvar_Get("j_side", "0.25", CVAR_ARCHIVE_ND);
	j_up      = Cvar_Get("j_up", "0", CVAR_ARCHIVE_ND);

	Cvar_CheckRange(j_pitch, -15.0f, 15.0f, qfalse);
	Cvar_CheckRange(j_yaw, -15.0f, 15.0f, qfalse);
	Cvar_CheckRange(j_forward, -15.0f, 15.0f, qfalse);
	Cvar_CheckRange(j_side, -15.0f, 15.0f, qfalse);
	Cvar_CheckRange(j_up, -15.0f, 15.0f, qfalse);

	j_pitch_axis   = Cvar_Get("j_pitch_axis", "3", CVAR_ARCHIVE_ND);
	j_yaw_axis     = Cvar_Get("j_yaw_axis", "2", CVAR_ARCHIVE_ND);
	j_forward_axis = Cvar_Get("j_forward_axis", "1", CVAR_ARCHIVE_ND);
	j_side_axis    = Cvar_Get("j_side_axis", "0", CVAR_ARCHIVE_ND);
	j_up_axis      = Cvar_Get("j_up_axis", "4", CVAR_ARCHIVE_ND);

	Cvar_CheckRange(j_pitch_axis, 0, MAX_JOYSTICK_AXIS - 1, qtrue);
	Cvar_CheckRange(j_yaw_axis, 0, MAX_JOYSTICK_AXIS - 1, qtrue);
	Cvar_CheckRange(j_forward_axis, 0, MAX_JOYSTICK_AXIS - 1, qtrue);
	Cvar_CheckRange(j_side_axis, 0, MAX_JOYSTICK_AXIS - 1, qtrue);
	Cvar_CheckRange(j_up_axis, 0, MAX_JOYSTICK_AXIS - 1, qtrue);

	// make these cvars visible to cgame
	cl_demorecording = Cvar_Get("cl_demorecording", "0", CVAR_ROM);
	cl_demofilename  = Cvar_Get("cl_demofilename", "", CVAR_ROM);
	cl_demooffset    = Cvar_Get("cl_demooffset", "0", CVAR_ROM);
	cl_waverecording = Cvar_Get("cl_waverecording", "0", CVAR_ROM);
	cl_wavefilename  = Cvar_Get("cl_wavefilename", "", CVAR_ROM);
	cl_waveoffset    = Cvar_Get("cl_waveoffset", "0", CVAR_ROM);

	cl_packetloss  = Cvar_Get("cl_packetloss", "0", CVAR_CHEAT);
	cl_packetdelay = Cvar_Get("cl_packetdelay", "0", CVAR_CHEAT);

	Cvar_Get("cl_maxPing", "800", CVAR_ARCHIVE_ND);

	// ~ and `, as keys and characters
	cl_consoleKeys = Cvar_Get("cl_consoleKeys", "~ ` 0x7e 0x60", CVAR_ARCHIVE);

	cl_interpolation = Cvar_GetAndDescribe("cl_interpolation", "0", CVAR_ARCHIVE,
	                                       "Buffering server packets to smooth over packetloss/ping instability.\nValues 0-4 depending on 'sv_fps' and 'snaps'.\nSet to 0 for most responsive gameplay.");
	Cvar_CheckRange(cl_interpolation, 0, 4, qtrue);

	Cvar_Get("cg_drawNotifyText", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_quickMessageAlt", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_popupLimboMenu", "1", CVAR_ARCHIVE);  // not used, kept for compatibility
	Cvar_Get("cg_drawTeamOverlay", "2", CVAR_ARCHIVE); // not used, kept for compatibility
	Cvar_Get("cg_drawGun", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_voiceSpriteTime", "6000", CVAR_ARCHIVE);
	Cvar_Get("cg_drawCrosshair", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_zoomDefaultSniper", "20", CVAR_ARCHIVE);
	Cvar_Get("cg_zoomStepSniper", "2", CVAR_ARCHIVE);

	// userinfo
	Cvar_Get("name", DEFAULT_NAME, CVAR_USERINFO | CVAR_ARCHIVE_ND);
	Cvar_Get("rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("etVersion", ET_VERSION, CVAR_USERINFO | CVAR_ROM);

	Cvar_Get("password", "", CVAR_USERINFO);
	Cvar_Get("cg_predictItems", "1", CVAR_ARCHIVE);

	Cvar_Get("cg_autoactivate", "1", CVAR_ARCHIVE);

	// cgame might not be initialized before menu is used
	Cvar_Get("cg_autoReload", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_weapaltReloads", "0", CVAR_ARCHIVE);
	Cvar_Get("cg_weapaltSwitches", "1", CVAR_ARCHIVE);
	Cvar_Get("cg_scopedSensitivityScaler", "0.6", CVAR_ARCHIVE);

	cl_missionStats = Cvar_Get("g_missionStats", "0", CVAR_ROM);

	// Auto-update
	com_updateavailable = Cvar_Get("com_updateavailable", "0", CVAR_ROM);
	com_updatefiles     = Cvar_Get("com_updatefiles", "", CVAR_ROM);
}
