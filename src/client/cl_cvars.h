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

#ifndef INCLUDE_CL_CVARS_H
#define INCLUDE_CL_CVARS_H

#include "../qcommon/q_shared.h"

void CL_InitCvars(void);

#ifdef USE_RENDERER_DLOPEN
extern cvar_t *cl_renderer;
#endif

extern cvar_t *cl_wavefilerecord;

extern cvar_t *rcon_client_password;
extern cvar_t *rconAddress;

extern cvar_t *cl_timeout;

extern cvar_t *cl_avidemotype;
extern cvar_t *cl_forceavidemo;

extern cvar_t *cl_wwwDownload;

extern cvar_t *cl_serverStatusResendTime;

extern cvar_t *cl_demorecording;
extern cvar_t *cl_demofilename;
extern cvar_t *cl_demooffset;

extern cvar_t *cl_waverecording;
extern cvar_t *cl_wavefilename;
extern cvar_t *cl_waveoffset;

extern cvar_t *cl_packetloss;
extern cvar_t *cl_packetdelay;

// cvars

extern cvar_t *cl_nodelta;
extern cvar_t *cl_debugMove;
extern cvar_t *cl_noprint;
extern cvar_t *cl_timegraph;
extern cvar_t *cl_maxpackets;
extern cvar_t *cl_packetdup;
extern cvar_t *cl_shownet;
extern cvar_t *cl_shownuments;
extern cvar_t *cl_showSend;
extern cvar_t *cl_showServerCommands;
extern cvar_t *cl_timeNudge;
extern cvar_t *cl_extrapolationMargin;
extern cvar_t *cl_showTimeDelta;
extern cvar_t *cl_freezeDemo;

extern cvar_t *cl_yawspeed;
extern cvar_t *cl_pitchspeed;
extern cvar_t *cl_run;
extern cvar_t *cl_anglespeedkey;

extern cvar_t *cl_recoilPitch;

extern cvar_t *cl_bypassMouseInput;

extern cvar_t *cl_doubletapdelay;

extern cvar_t *cl_sensitivity;
extern cvar_t *cl_freelook;

extern cvar_t *cl_mouseAccel;
extern cvar_t *cl_showMouseRate;

extern cvar_t *cl_avidemo;
extern cvar_t *cl_aviMotionJpeg;
extern cvar_t *cl_aviFrameRate;
extern cvar_t *cl_aviPipeFormat;
extern cvar_t *cl_aviPipeExtension;

extern cvar_t *m_pitch;
extern cvar_t *m_yaw;
extern cvar_t *m_forward;
extern cvar_t *m_side;
extern cvar_t *m_filter;

extern cvar_t *j_pitch;
extern cvar_t *j_yaw;
extern cvar_t *j_forward;
extern cvar_t *j_side;
extern cvar_t *j_up;
extern cvar_t *j_pitch_axis;
extern cvar_t *j_yaw_axis;
extern cvar_t *j_forward_axis;
extern cvar_t *j_side_axis;
extern cvar_t *j_up_axis;

extern cvar_t *cl_timedemo;

extern cvar_t *cl_activeAction;
extern cvar_t *cl_autorecord;

extern cvar_t *cl_allowDownload;
extern cvar_t *cl_conXOffset;

extern cvar_t *cl_missionStats;

extern cvar_t *cl_profile;
extern cvar_t *cl_defaultProfile;

extern cvar_t *cl_consoleKeys;

extern cvar_t *cl_interpolation;

extern cvar_t *con_scale;

#endif  // #ifndef INCLUDE_CL_CVARS_H
