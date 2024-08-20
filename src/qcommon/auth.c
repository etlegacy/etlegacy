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
 * @file auth.c
 * @brief Handle central authentication for ET:Legacy
 */

#ifndef DEDICATED
#include "../client/client.h"
#endif
#include "../server/server.h"

#include "json.h"

#ifdef DEDICATED
#define AUTH_TOKEN_FILE "sv-token.dat"
#else
#define AUTH_TOKEN_FILE "cl-token.dat"
#endif

#define AUTH_TOKEN_SIZE 65
#define AUTH_USERNAME_SIZE 65

static struct
{
	char authToken[AUTH_TOKEN_SIZE];
	char username[AUTH_USERNAME_SIZE];
} authData = { { 0 }, { 0 } };

cvar_t *auth_server;
cvar_t *sv_auth;

#define AUTH_DEFAULT_SERVER "https://master.etlegacy.com"

#define AUTH_CLIENT_LOGIN "/api/client/login"
#define AUTH_CLIENT_CHALLENGE "/api/client/auth/challenge"
#define AUTH_SERVER_CHALLENGE "/api/server/auth/challenge"
#define AUTH_SERVER_VERIFY "/api/server/auth/verify"

#define A_URL(x) va("%s" x, auth_server->string)

#ifndef DEDICATED
static void Auth_SendToServer(const char *format, ...)
{
	static char string[128];
	va_list     argPtr;

	if (cls.state < CA_CONNECTED || cls.state > CA_ACTIVE)
	{
		return;
	}

	va_start(argPtr, format);
	Q_vsnprintf(string, 128, format, argPtr);
	va_end(argPtr);

	CL_AddReliableCommand(string);
}
#else
#define Auth_SendToServer(...)
#endif

#define Auth_SendToClient(client, ...) SV_SendServerCommand((client), __VA_ARGS__)

qboolean Auth_SV_RemoveAuthFromUserinfo(char *userinfo)
{
	qboolean ret = qfalse;

	if (Info_RemoveKey(userinfo, "auth"))
	{
		ret = qtrue;
	}

	if (Info_RemoveKey(userinfo, "authId"))
	{
		ret = qtrue;
	}

	return ret;
}

void Auth_SV_SetUserinfoAuth(void *gameClient)
{
	client_t *client = gameClient;

	if (!client->loginId)
	{
		return;
	}

	if (!strstr(client->userinfo, "auth\\"))
	{
		Info_SetValueForKey(client->userinfo, "auth", client->login);
	}

	if (!strstr(client->userinfo, "authId\\"))
	{
		Info_SetValueForKey(client->userinfo, "authId", va("%i", client->loginId));
	}
}

static void Auth_SV_UserInfoChanged(client_t *client)
{
	VM_Call(gvm, GAME_CLIENT_USERINFO_CHANGED, client - svs.clients);
}

static void Auth_SV_ChallengeReceived(client_t *client, const char *challenge)
{
	if (!challenge || !*challenge)
	{
		// FIXME: message this..
		return;
	}

	Q_strncpyz(client->loginChallenge, challenge, sizeof(client->loginChallenge));
	Auth_SendToClient(client, "//auth-srv challenge %s", client->loginChallenge);
	client->loginStatus = LOGIN_CLIENT_CHALLENGED;
}

static void Auth_SV_ResponseVerify(client_t *client, qboolean match, uint32_t userId)
{
	if (!match)
	{
		client->login[0]          = '\0';
		client->loginChallenge[0] = '\0';
		client->loginStatus       = LOGIN_NONE;
		client->loginId           = 0;
		if (Auth_SV_RemoveAuthFromUserinfo(client->userinfo))
		{
			Auth_SV_UserInfoChanged(client);
		}
	}
	else
	{
		client->loginStatus = LOGIN_CLIENT_LOGGED_IN;
		client->loginId     = userId;
		Info_SetValueForKey(client->userinfo, "auth", client->login);
		Info_SetValueForKey(client->userinfo, "authId", va("%i", client->loginId));
		Auth_SV_UserInfoChanged(client);
	}
	Auth_SendToClient(client, "//auth-srv verified %i", match);
}

static void Auth_FreeUploadBuffer(webRequest_t *request)
{
	if (request->uploadData)
	{
		if (request->uploadData->buffer)
		{
			Com_Dealloc(request->uploadData->buffer);
		}
		Com_Dealloc(request->uploadData);
		request->uploadData = NULL;
	}
}

#ifndef DEDICATED
static void Auth_WriteTokenToFile(const char *token)
{
	fileHandle_t f;

	if (!token || !*token)
	{
		return;
	}

	f = FS_SV_FOpenFileWrite(AUTH_TOKEN_FILE);
	if (!f)
	{
		Com_DPrintf("NOTE: Could not open %s for write\n", AUTH_TOKEN_FILE);
		return;
	}

	FS_Write(token, (int)strlen(token), f);
	FS_FCloseFile(f);
}

static void Auth_ClientLoginCallback(struct webRequest_s *request, webRequestResult requestResult)
{
	cJSON *root = NULL;

	Auth_FreeUploadBuffer(request);

	if (!requestResult)
	{
		return;
	}

	root = cJSON_Parse((char *)request->data.buffer);

	if (cJSON_HasObjectItem(root, "token") && cJSON_HasObjectItem(root, "username"))
	{
		authData.authToken[0] = '\0';
		Q_strncpyz(authData.authToken, cJSON_GetStringValue(cJSON_GetObjectItem(root, "token")), AUTH_TOKEN_SIZE);

		authData.username[0] = '\0';
		Q_strncpyz(authData.username, cJSON_GetStringValue(cJSON_GetObjectItem(root, "username")), AUTH_USERNAME_SIZE);

		Auth_WriteTokenToFile(authData.authToken);

		Auth_SendToServer("login %s", authData.username);
	}

	cJSON_free(root);
}

static void Auth_ClientLogin(const char *username, const char *password)
{
	char            *tmp;
	cJSON           *root;
	webUploadData_t *upload = NULL;

	upload = Com_Allocate(sizeof(webUploadData_t));
	Com_Memset(upload, 0, sizeof(webUploadData_t));

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "username", username);
	cJSON_AddStringToObject(root, "password", password);
	cJSON_AddStringToObject(root, "guid", Cvar_VariableString("cl_guid"));
	tmp = cJSON_PrintUnformatted(root);
	cJSON_free(root);
	Q_strncpyz(upload->contentType, "application/json", sizeof(upload->contentType));
	upload->buffer     = (byte *)tmp;
	upload->bufferSize = strlen(tmp);

	Web_CreateRequest(A_URL(AUTH_CLIENT_LOGIN), NULL, upload, NULL, &Auth_ClientLoginCallback, NULL);
}

static void Auth_ClientLoginTestCallback(struct webRequest_s *request, webRequestResult requestResult)
{
	cJSON *root = NULL;

	if (!requestResult)
	{
		authData.authToken[0] = '\0';
		Com_Printf(S_COLOR_RED "Could not log in with token\n");
	}

	root = cJSON_Parse((char *)request->data.buffer);

	if (cJSON_HasObjectItem(root, "username"))
	{
		authData.username[0] = '\0';
		Q_strncpyz(authData.username, cJSON_GetStringValue(cJSON_GetObjectItem(root, "username")), AUTH_USERNAME_SIZE);
		Com_Printf("Logged in as %s\n", authData.username);
	}

	cJSON_free(root);
}

static void Auth_TestToken(void)
{
	Web_CreateRequest(A_URL(AUTH_CLIENT_LOGIN), authData.authToken, NULL, NULL, &Auth_ClientLoginTestCallback, NULL);
}
#endif

static void Auth_ReadToken(void)
{
	long         len;
	fileHandle_t f = 0;

	Com_Memset(authData.authToken, 0, AUTH_TOKEN_SIZE);

	len = FS_SV_FOpenFileRead(AUTH_TOKEN_FILE, &f);
	if (!f)
	{
		Com_DPrintf("NOTE: Could not open %s for read\n", AUTH_TOKEN_FILE);
		return;
	}

	if (len >= AUTH_TOKEN_SIZE || len < 20)
	{
		FS_FCloseFile(f);
		return;
	}

	FS_Read(authData.authToken, AUTH_TOKEN_SIZE, f);

	FS_FCloseFile(f);
}

#ifndef DEDICATED
static void Auth_ClearToken(void)
{
	fileHandle_t f;

	authData.authToken[0] = '\0';

	f = FS_SV_FOpenFileWrite(AUTH_TOKEN_FILE);
	if (!f)
	{
		Com_DPrintf("NOTE: Could not open %s for write\n", AUTH_TOKEN_FILE);
		return;
	}

	FS_Write("NOP", 3, f);

	FS_FCloseFile(f);
}

static void Auth_Cmd_f(void)
{
	char *tmp  = NULL;
	int  count = Cmd_Argc();

	if (!*auth_server->string)
	{
		Com_Printf("Missing auth server\n");
		return;
	}

	if (count < 2)
	{
		Com_Printf("Requires arguments\n");
		return;
	}

	tmp = Cmd_Argv(1);

	if (!Q_stricmp(tmp, "login"))
	{
		if (*authData.authToken && *authData.username)
		{
			Auth_SendToServer("login %s", authData.username);
			// Com_Printf("Already logged in\n");
			return;
		}

		if (count < 4)
		{
			Com_Printf("username and password are required\n");
			return;
		}

		Auth_ClientLogin(Cmd_Argv(2), Cmd_Argv(3));
		return;
	}

	if (!Auth_Active())
	{
		Com_Printf("Not logged in\n");
		return;
	}

	if (!Q_stricmp(tmp, "logout"))
	{
		Auth_ClearToken();
		Auth_SendToServer("logout");
	}
	else
	{
		Com_Printf("Unknown command: %s\n", tmp);
	}
}

void Auth_Server_Command_f(void)
{
	char *tmp  = NULL;
	int  count = Cmd_Argc();

	if (!Auth_Active())
	{
		return;
	}

	if (count < 2)
	{
		Com_Printf("Server auth missing arguments\n");
	}

	tmp = Cmd_Argv(1);
	if (!Q_stricmp(tmp, "prompt"))
	{
		if (*authData.authToken && *authData.username)
		{
			Auth_SendToServer("login %s", authData.username);
		}
		else
		{
			Com_Printf("Need to send login info to the server\n");
		}
	}
	else if (!Q_stricmp(tmp, "challenge"))
	{
		if (count != 3)
		{
			Com_Printf("Invalid server challenge message\n");
			return;
		}
		if (*authData.authToken && *authData.username)
		{
			Auth_Client_FetchResponse(Cmd_Argv(2));
		}
		else
		{
			Com_Printf("Need to send challenge response to the server\n");
		}
	}
	else if (!Q_stricmp(tmp, "verified"))
	{
		int verStatus;

		if (count != 3)
		{
			Com_Printf("Invalid server verified message\n");
			return;
		}

		verStatus = Q_atoi(Cmd_Argv(2));

		if (verStatus == 1)
		{
			Com_Printf(S_COLOR_CYAN "Logged in the server as %s\n", authData.username);
		}
		else
		{
			Com_Printf(S_COLOR_CYAN "Authentication failed as %s\n", authData.username);
		}
	}
	else if (!Q_stricmp(tmp, "logout"))
	{
		if (count != 3)
		{
			Com_Printf("Invalid server logout message\n");
			return;
		}

		Com_Printf(S_COLOR_CYAN "Logged out of the server as %s\n", Cmd_Argv(2));
	}
}

static void Auth_ClientChallengeCallback(struct webRequest_s *request, webRequestResult requestResult)
{
	cJSON *root = NULL;

	Auth_FreeUploadBuffer(request);

	if (requestResult != REQUEST_OK)
	{
		return;
	}

	root = cJSON_Parse((char *)request->data.buffer);

	if (cJSON_HasObjectItem(root, "response"))
	{
		char *response = cJSON_GetStringValue(cJSON_GetObjectItem(root, "response"));
		Auth_SendToServer("login-response %s", response);
	}

	cJSON_free(root);
}
#endif

static void Auth_ServerChallengeCallback(struct webRequest_s *request, webRequestResult requestResult)
{
	cJSON *root = NULL;

	Auth_FreeUploadBuffer(request);

	if (requestResult != REQUEST_OK)
	{
		return;
	}

	root = cJSON_Parse((char *)request->data.buffer);

	if (cJSON_HasObjectItem(root, "challenge"))
	{
		char *challenge = cJSON_GetStringValue(cJSON_GetObjectItem(root, "challenge"));
		Auth_SV_ChallengeReceived(request->userData, challenge);
	}

	cJSON_free(root);
}

static void Auth_ServerVerifyCallback(struct webRequest_s *request, webRequestResult requestResult)
{
	cJSON    *root = NULL, *tmp = NULL;
	qboolean match = qfalse;
	uint32_t id    = 0;

	Auth_FreeUploadBuffer(request);

	if (requestResult != REQUEST_OK)
	{
		return;
	}

	root = cJSON_Parse((char *)request->data.buffer);

	if (cJSON_HasObjectItem(root, "match"))
	{
		tmp   = cJSON_GetObjectItem(root, "match");
		match = cJSON_IsBool(tmp) && cJSON_IsTrue(tmp);
	}
	if (cJSON_HasObjectItem(root, "userId"))
	{
		tmp = cJSON_GetObjectItem(root, "userId");
		if (cJSON_IsNumber(tmp))
		{
			id = (uint32_t)cJSON_GetNumberValue(tmp);
		}
	}

	Auth_SV_ResponseVerify(request->userData, match, id);

	cJSON_free(root);
}

void Auth_Server_ClientLogout(void *data, const char *username)
{
	client_t *client = (client_t *)data;

	if (client->loginStatus == LOGIN_CLIENT_LOGGED_IN)
	{
		Auth_SendToClient(data, "//auth-srv logout %s", username);
	}

	client->login[0]          = '\0';
	client->loginChallenge[0] = '\0';
	client->loginId           = 0;
	client->loginStatus       = LOGIN_NONE;
	client->loginRequested    = 0;
	if (Auth_SV_RemoveAuthFromUserinfo(client->userinfo))
	{
		Auth_SV_UserInfoChanged(client);
	}
}

void Auth_Server_VerifyResponse(void *data, const char *username, const char *challenge, const char *response)
{
	cJSON *root;
	char  *json;

	webUploadData_t *upload = NULL;

	upload = Com_Allocate(sizeof(webUploadData_t));
	Com_Memset(upload, 0, sizeof(webUploadData_t));

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "username", username);
	cJSON_AddStringToObject(root, "challenge", challenge);
	cJSON_AddStringToObject(root, "response", response);
	json = cJSON_PrintUnformatted(root);
	cJSON_free(root);

	Q_strncpyz(upload->contentType, "application/json", sizeof(upload->contentType));
	upload->buffer     = (byte *)json;
	upload->bufferSize = strlen(json);

	((client_t *)data)->loginStatus = LOGIN_SERVER_VERIFY;

	Web_CreateRequest(A_URL(AUTH_SERVER_VERIFY), authData.authToken, upload, data, &Auth_ServerVerifyCallback, NULL);
}

void Auth_Server_FetchChallenge(void *data, const char *username)
{
	cJSON *root;
	char  *json;

	webUploadData_t *upload = NULL;

	if (data)
	{
		((client_t *)data)->loginStatus = LOGIN_SERVER_CHALLENGED;
	}
	else
	{
		Com_Printf(S_COLOR_RED "ERROR: No client data\n");
		return;
	}

	upload = Com_Allocate(sizeof(webUploadData_t));
	Com_Memset(upload, 0, sizeof(webUploadData_t));

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "username", username);
	json = cJSON_PrintUnformatted(root);
	cJSON_free(root);

	Q_strncpyz(upload->contentType, "application/json", sizeof(upload->contentType));
	upload->buffer     = (byte *)json;
	upload->bufferSize = strlen(json);

	Web_CreateRequest(A_URL(AUTH_SERVER_CHALLENGE), authData.authToken, upload, data, &Auth_ServerChallengeCallback, NULL);
}

void Auth_Server_RequestClientAuthentication(void *data)
{
	client_t *client = data;

	if (sv_auth->integer && !client->loginStatus)
	{
		client->loginRequested = svs.time;
		client->loginStatus    = LOGIN_SERVER_REQUESTED;
		Auth_SendToClient(data, "//auth-srv prompt");

		if (!(client->agent.compatible & BIT(2)))
		{
			// client is not authentication "aware" so we need to send a prompt instead
			Auth_SendToClient(data, "authMsg \"^7You need to ^1authenticate ^7via ^3www.etlegacy.com\"");
		}
	}
}

qboolean Auth_Server_AuthenticationRequired(void)
{
	if (sv_auth->integer == 2)
	{
		return Auth_Active();
	}

	return qfalse;
}

#ifndef DEDICATED
void Auth_Client_FetchResponse(const char *challenge)
{
	cJSON *root;
	char  *json;

	webUploadData_t *upload = NULL;

	upload = Com_Allocate(sizeof(webUploadData_t));
	Com_Memset(upload, 0, sizeof(webUploadData_t));

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "challenge", challenge);
	cJSON_AddStringToObject(root, "guid", Cvar_VariableString("cl_guid"));
	json = cJSON_PrintUnformatted(root);
	cJSON_free(root);

	Q_strncpyz(upload->contentType, "application/json", sizeof(upload->contentType));
	upload->buffer     = (byte *)json;
	upload->bufferSize = strlen(json);

	Web_CreateRequest(A_URL(AUTH_CLIENT_CHALLENGE), authData.authToken, upload, NULL, &Auth_ClientChallengeCallback, NULL);
}
#endif

qboolean Auth_Active(void)
{
#ifdef DEDICATED
	if (*authData.authToken && *auth_server->string)
	{
		return qtrue;
	}
#else
	if (*authData.authToken && *authData.username && *auth_server->string)
	{
		return qtrue;
	}
#endif
	return qfalse;
}

void Auth_Init(void)
{
	Com_Memset(&authData, 0, sizeof(authData));

	auth_server = Cvar_Get("auth_server", AUTH_DEFAULT_SERVER, CVAR_INIT | CVAR_PROTECTED | CVAR_NOTABCOMPLETE);
	sv_auth     = Cvar_Get("sv_auth", "0", CVAR_ARCHIVE_ND);

	if (!*auth_server->string)
	{
		return;
	}

#ifndef DEDICATED
	Cmd_AddCommand("auth", Auth_Cmd_f, "Authentication handler");
#ifdef ETLEGACY_DEBUG
	Cmd_AddCommand("authTest", Auth_TestToken, "Test auth token manually");
#endif
#endif

	Auth_ReadToken();

#ifndef DEDICATED
	if (*authData.authToken)
	{
		Auth_TestToken();
	}
#endif
}

void Auth_Shutdown(void)
{
#ifndef DEDICATED
	Cmd_RemoveCommand("auth");
#endif
}
