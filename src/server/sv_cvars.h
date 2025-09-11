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

#ifndef INCLUDE_SV_CVARS_H
#define INCLUDE_SV_CVARS_H

#include "../qcommon/q_shared.h"

extern cvar_t *sv_fps;
extern cvar_t *sv_timeout;
extern cvar_t *sv_zombietime;
extern cvar_t *sv_rconPassword;
extern cvar_t *sv_privatePassword;
extern cvar_t *sv_hidden;
extern cvar_t *sv_allowDownload;
extern cvar_t *sv_friendlyFire;
extern cvar_t *sv_maxlives;
extern cvar_t *sv_maxclients;
extern cvar_t *sv_needpass;
extern cvar_t *sv_democlients; ///< number of democlients: this should always be set to 0,
                               ///< and will be automatically adjusted when needed by the demo facility.
                               ///< ATTENTION: if sv_maxclients = sv_democlients then server will be full!
                               ///< sv_democlients consume clients slots even if there are no democlients recorded nor replaying for this slot!

extern cvar_t *sv_privateClients;
extern cvar_t *sv_hostname;
extern cvar_t *sv_master[MAX_MASTER_SERVERS];

#ifdef FEATURE_TRACKER
extern cvar_t *sv_tracker;
#endif

extern cvar_t *sv_reconnectlimit;
extern cvar_t *sv_tempbanmessage;

extern cvar_t *sv_padPackets;
extern cvar_t *sv_killserver;
extern cvar_t *sv_mapname;
extern cvar_t *sv_mapChecksum;
extern cvar_t *sv_serverid;
extern cvar_t *sv_minRate;
extern cvar_t *sv_maxRate;
extern cvar_t *sv_minPing;
extern cvar_t *sv_maxPing;
extern cvar_t *sv_gametype;
extern cvar_t *sv_pure;
extern cvar_t *sv_floodProtect;
extern cvar_t *sv_userInfoFloodProtect;
extern cvar_t *sv_lanForceRate;
extern cvar_t *sv_onlyVisibleClients;

extern cvar_t *sv_showAverageBPS;           ///< net debugging

/// autodl
extern cvar_t *sv_dl_timeout;

extern cvar_t *sv_wwwDownload; ///< general flag to enable/disable www download redirects
extern cvar_t *sv_wwwBaseURL;  ///< the base URL of all the files
/// tell clients to perform their downloads while disconnected from the server
/// this gets you a better throughput, but you loose the ability to control the download usage
extern cvar_t *sv_wwwDlDisconnected;
extern cvar_t *sv_wwwFallbackURL;

extern cvar_t *sv_cheats;
extern cvar_t *sv_packetloss;
extern cvar_t *sv_packetdelay;

extern cvar_t *sv_dlRate;

extern cvar_t *sv_fullmsg;

extern cvar_t *sv_advert;

extern cvar_t *sv_protect;
extern cvar_t *sv_protectLog;
extern cvar_t *sv_protectLogInterval;

#ifdef FEATURE_ANTICHEAT
extern cvar_t *sv_wh_active;
extern cvar_t *sv_wh_bbox_horz;
extern cvar_t *sv_wh_bbox_vert;
extern cvar_t *sv_wh_check_fov;
#endif

// server side demo recording
extern cvar_t *sv_demopath;
extern cvar_t *sv_demoState;
extern cvar_t *sv_autoDemo;
extern cvar_t *sv_freezeDemo;
extern cvar_t *sv_demoTolerant;

extern cvar_t *sv_ipMaxClients; ///< limit client connection

extern cvar_t *sv_serverTimeReset;

extern cvar_t *sv_etltv_maxslaves;
extern cvar_t *sv_etltv_password;
extern cvar_t *sv_etltv_autorecord;
extern cvar_t *sv_etltv_autoplay;
extern cvar_t *sv_etltv_clientname;
extern cvar_t *sv_etltv_delay;
extern cvar_t *sv_etltv_shownet;
extern cvar_t *sv_etltv_queue_ms;
extern cvar_t *sv_etltv_netblast;

#endif  // #ifndef INCLUDE_SV_CVARS_H
