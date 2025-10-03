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
#include "sv_cvars.h"

cvar_t *sv_fps = NULL;          // time rate for running non-clients
cvar_t *sv_timeout;             // seconds without any message
cvar_t *sv_zombietime;          // seconds to sink messages after disconnect
cvar_t *sv_rconPassword;        // password for remote server commands
cvar_t *sv_privatePassword;     // password for the privateClient slots
cvar_t *sv_hidden;
cvar_t *sv_allowDownload;
cvar_t *sv_maxclients;
cvar_t *sv_democlients;         // number of slots reserved for playing a demo

cvar_t *sv_privateClients;      // number of clients reserved for password
cvar_t *sv_hostname;
cvar_t *sv_reconnectlimit;      // minimum seconds between connect messages
cvar_t *sv_tempbanmessage;

cvar_t *sv_padPackets;          // add nop bytes to messages
cvar_t *sv_killserver;          // menu system can set to 1 to shut server down
cvar_t *sv_mapname;
cvar_t *sv_mapChecksum;
cvar_t *sv_serverid;
cvar_t *sv_minRate;
cvar_t *sv_maxRate;
cvar_t *sv_minPing;
cvar_t *sv_maxPing;
cvar_t *sv_gametype;
cvar_t *sv_pure;
cvar_t *sv_floodProtect;
cvar_t *sv_userInfoFloodProtect;
cvar_t *sv_lanForceRate;        // dedicated 1 (LAN) server forces local client rates to 99999 (bug #491)
cvar_t *sv_onlyVisibleClients;
cvar_t *sv_friendlyFire;
cvar_t *sv_maxlives;
cvar_t *sv_needpass;

cvar_t *sv_dl_timeout;          // seconds without any message when cl->state != CS_ACTIVE

cvar_t *sv_showAverageBPS;      // net debugging

cvar_t *sv_wwwDownload;         // server does a www dl redirect
cvar_t *sv_wwwBaseURL;          // base URL for redirect
// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you loose the ability to control the download usage
cvar_t *sv_wwwDlDisconnected;
cvar_t *sv_wwwFallbackURL; // URL to send to if an http/ftp fails or is refused client side

cvar_t *sv_cheats;
cvar_t *sv_packetloss;
cvar_t *sv_packetdelay;

cvar_t *sv_fullmsg;

cvar_t *sv_dlRate;

// do we communicate with others ?
cvar_t *sv_advert;      // 0 - no big brothers
                        // 1 - communicate with master server
                        // 2 - communicate with tracker

// server attack protection
cvar_t *sv_protect;     // 0 - unprotected
                        // 1 - ioquake3 method (default)
                        // 2 - OpenWolf method
                        // 4 - prints attack info to console (when ioquake3 or OPenWolf method is set)
cvar_t *sv_protectLog;  // name of log file
cvar_t *sv_protectLogInterval; // how often to write attack log entries

#ifdef FEATURE_ANTICHEAT
cvar_t *sv_wh_active;
cvar_t *sv_wh_bbox_horz;
cvar_t *sv_wh_bbox_vert;
cvar_t *sv_wh_check_fov;
#endif

cvar_t *sv_demopath;
cvar_t *sv_demoState;
cvar_t *sv_autoDemo;
cvar_t *sv_freezeDemo;  // to freeze server-side demos
cvar_t *sv_demoTolerant;

cvar_t *sv_ipMaxClients;

cvar_t *sv_serverTimeReset;

cvar_t *sv_etltv_maxslaves;
cvar_t *sv_etltv_password;
cvar_t *sv_etltv_autorecord;
cvar_t *sv_etltv_autoplay;
cvar_t *sv_etltv_clientname;
cvar_t *sv_etltv_delay;
cvar_t *sv_etltv_shownet;
cvar_t *sv_etltv_queue_ms;
cvar_t *sv_etltv_netblast;
