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
 * If not, please request a copy in writing to id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 */
/**
 * @file cl_discord.c
 * @brief Discord Rich Presence client wrapper
 *
 * @note This is FEATURE_DISCORD and _WIN32 only
 */

// this is FEATURE_DISCORD and _WIN32 only

#include "../client/client.h"
#include "cl_discord.h"

#ifdef _WIN32
#include "discord_rpc.h"

#define DISCORD_PRESENCE_UPDATE_INTERVAL 15000   /**< ms — throttle for Discord_UpdatePresence (FR #6) */
#define DISCORD_LARGE_IMAGE_KEY          "etlegacy_logo"

/* file-scope state */
static qboolean    drp_active             = qfalse; /**< Discord_Initialize called and not torn down */
static int         drp_lastPresence       = -1;     /**< cached cl_discordPresence->integer (live-toggle diff, D8) */
static int         drp_lastUpdate         = 0;      /**< cls.realtime of last presence push (+1 idiom, D14) */
static connstate_t drp_prevState          = CA_UNINITIALIZED; /**< for startTimestamp transition polling, D13 */
static int64_t     drp_startTime          = 0;      /**< startTimestamp (time(NULL)); 0 = no elapsed timer */
static qboolean    drp_erroredLogged      = qfalse; /**< log-once for errored() (D15) */
static qboolean    drp_disconnectedLogged = qfalse; /**< log-once for disconnected() (D15) */

/* stable backing buffers — the SDK copies internally, but we keep these static so
   the const char* pointers stay valid; Info_ValueForKey uses a shared static
   double-buffer (q_shared.c:2542), so we copy out before populating the struct */
static char drp_state[128];         /**< ≤128 (discord_rpc.h:27) */
static char drp_details[128];        /**< ≤128 */
static char drp_largeImageText[128]; /**< ≤128 */
static char drp_largeImageKey[32];   /**< ≤32  (discord_rpc.h:31) */

/**
 * @brief DRP_OnReady
 * @param[in] request
 */
static void DRP_OnReady(const DiscordUser *request)
{
	(void)request;
	Com_Printf("Discord: ready\n");
}

/**
 * @brief DRP_OnDisconnected — log once (D15)
 */
static void DRP_OnDisconnected(int errorCode, const char *message)
{
	if (!drp_disconnectedLogged)
	{
		Com_Printf(S_COLOR_YELLOW "Discord: disconnected (%i: %s)\n", errorCode, message ? message : "");
		drp_disconnectedLogged = qtrue;
	}
}

/**
 * @brief DRP_OnErrored — log once (D15)
 */
static void DRP_OnErrored(int errorCode, const char *message)
{
	if (!drp_erroredLogged)
	{
		Com_Printf(S_COLOR_YELLOW "Discord: error (%i: %s)\n", errorCode, message ? message : "");
		drp_erroredLogged = qtrue;
	}
}

/**
 * @brief DRP_UpdatePresence — build the DiscordRichPresence payload from cls/cl state + push it.
 */
static void DRP_UpdatePresence(void)
{
	DiscordRichPresence presence;

	Com_Memset(&presence, 0, sizeof(presence));

	Q_strncpyz(drp_largeImageKey, DISCORD_LARGE_IMAGE_KEY, sizeof(drp_largeImageKey));

	if (cls.state >= CA_CONNECTED && cls.state != CA_CINEMATIC)
	{
		const char *info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
		const char *s;

		/* map name → details (direct CS_SERVERINFO read, no stripping — D12) */
		s = Info_ValueForKey(info, "mapname");
		if (s && *s)
		{
			Q_strncpyz(drp_details, s, sizeof(drp_details));
		}
		else
		{
			drp_details[0] = '\0';
		}

		/* server hostname → largeImageText */
		s = Info_ValueForKey(info, "sv_hostname");
		if (s && *s)
		{
			Q_strncpyz(drp_largeImageText, s, sizeof(drp_largeImageText));
		}
		else
		{
			Q_strncpyz(drp_largeImageText, "ET: Legacy", sizeof(drp_largeImageText));
		}

		if (cls.state == CA_ACTIVE && cl.snap.valid)
		{
			int        team     = cl.snap.ps.persistant[PERS_TEAM];
			const char *teamStr = "";

			switch (team)
			{
			case TEAM_AXIS:      teamStr = " (Axis)"; break;
			case TEAM_ALLIES:    teamStr = " (Allies)"; break;
			case TEAM_SPECTATOR: teamStr = " (Spectator)"; break;
			default:             teamStr = ""; break;   /* TEAM_FREE / out-of-range → "In Game" (D11) */
			}

			Com_sprintf(drp_state, sizeof(drp_state), "In Game%s", teamStr);
			presence.startTimestamp = drp_startTime;   /* time(NULL) captured on entering CA_ACTIVE (D13) */
		}
		else
		{
			/* CA_CONNECTED / CA_LOADING / CA_PRIMED */
			Q_strncpyz(drp_state, "Connecting", sizeof(drp_state));
		}
	}
	else
	{
		/* menu / cinematic / disconnected */
		Q_strncpyz(drp_state, "In Menu", sizeof(drp_state));
		Q_strncpyz(drp_largeImageText, "ET: Legacy", sizeof(drp_largeImageText));
		drp_details[0] = '\0';
	}

	presence.state          = drp_state;
	presence.details        = drp_details;
	presence.largeImageKey  = drp_largeImageKey;
	presence.largeImageText = drp_largeImageText;
	presence.instance       = 0;   /* not a joinable instance (no Ask-to-Join) */

	Discord_UpdatePresence(&presence);
}

/**
 * @brief DRP_Startup — initialize the SDK + push initial presence.
 * @return qtrue if activated; qfalse if no applicationId is set (D17).
 */
static qboolean DRP_Startup(void)
{
	DiscordEventHandlers handlers;

	/* applicationId is a one-time dashboard prerequisite; until set, do not activate (D17) */
	if (!cl_discordAppId->string || !cl_discordAppId->string[0])
	{
		return qfalse;
	}

	Com_Memset(&handlers, 0, sizeof(handlers));
	handlers.ready        = DRP_OnReady;
	handlers.disconnected = DRP_OnDisconnected;
	handlers.errored      = DRP_OnErrored;
	/* joinGame / spectateGame / joinRequest stay NULL (non-goals, D15) */

	Discord_Initialize(cl_discordAppId->string, &handlers, 0 /*autoRegister*/, NULL /*optionalSteamId*/);

	drp_active             = qtrue;
	drp_prevState          = cls.state;        /* seed to avoid a spurious transition this frame */
	drp_lastUpdate         = cls.realtime + 1;  /* next throttled push 15 s after this immediate push */
	drp_erroredLogged      = qfalse;
	drp_disconnectedLogged = qfalse;

	/* if (re-)initializing while already in-game, capture the start time now */
	drp_startTime = (cls.state == CA_ACTIVE && cl.snap.valid) ? (int64_t)time(NULL) : 0;

	DRP_UpdatePresence();   /* push initial presence immediately */
	return qtrue;
}

/**
 * @brief DRP_Teardown — clear presence + shut down the SDK (full teardown, D9).
 */
static void DRP_Teardown(void)
{
	if (!drp_active)
	{
		return;
	}

	Discord_ClearPresence();
	Discord_Shutdown();

	drp_active    = qfalse;
	drp_startTime = 0;
}

/**
 * @brief CL_DiscordInit — called from CL_Init after CL_InitCvars.
 */
void CL_DiscordInit(void)
{
	drp_lastPresence = cl_discordPresence->integer;

	if (drp_lastPresence)
	{
		if (!DRP_Startup())
		{
			Com_Printf(S_COLOR_YELLOW "Discord: no applicationId set (cl_discordAppId), Rich Presence disabled\n");
		}
	}
}

/**
 * @brief CL_DiscordFrame — called from CL_Frame every frame.
 */
void CL_DiscordFrame(void)
{
	int presence = cl_discordPresence->integer;

	/* live-toggle detection: manual int diff (D8) */
	if (presence != drp_lastPresence)
	{
		if (presence && !drp_active)
		{
			/* 0→1: (re-)initialize (no-op if no applicationId) */
			DRP_Startup();
		}
		else if (!presence && drp_active)
		{
			/* 1→0: full teardown (D9) */
			DRP_Teardown();
		}
		drp_lastPresence = presence;
	}

	if (!drp_active)
	{
		/* disabled, or no applicationId — nothing to do */
		return;
	}

	/* drain the in-process callback queue every frame (cheap; the SDK's own IO
	   thread does the network + reconnects — do NOT use DISCORD_DISABLE_IO_THREAD) */
	Discord_RunCallbacks();

	/* startTimestamp state-transition polling — catches ALL CA_ACTIVE / CA_DISCONNECTED
	   paths (cl_cgame.c:1566, cl_demo.c:870, CL_Disconnect, CL_Connect_f, cinematic)
	   without modifying those files (D13) */
	if (drp_prevState != CA_ACTIVE && cls.state == CA_ACTIVE)
	{
		drp_startTime = (int64_t)time(NULL);
	}
	else if (drp_prevState != CA_DISCONNECTED && cls.state == CA_DISCONNECTED)
	{
		drp_startTime = 0;
	}
	drp_prevState = cls.state;

	/* throttle Discord_UpdatePresence to one push per 15 s (+1 first-frame idiom,
	   CL_RequestMasterData cl_main.c:732) (D14) */
	if (cls.realtime - drp_lastUpdate < DISCORD_PRESENCE_UPDATE_INTERVAL)
	{
		return;
	}
	drp_lastUpdate = cls.realtime + 1;

	DRP_UpdatePresence();
}

/**
 * @brief CL_DiscordShutdown — called from CL_Shutdown before the client state is zeroed.
 */
void CL_DiscordShutdown(void)
{
	DRP_Teardown();
}

#endif /* _WIN32 */
