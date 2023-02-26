/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2023 ET:Legacy team <mail@etlegacy.com>
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

#ifdef DEDICATED
#include "../server/server.h"
#else
#include "../client/client.h"
#endif

#include "json.h"

#define AUTH_TOKEN_FILE "auth-token.dat"
#define AUTH_TOKEN_SIZE 65
#define AUTH_USERNAME_SIZE 65

static struct
{
	char authToken[AUTH_TOKEN_SIZE];
	char username[AUTH_USERNAME_SIZE];
} authData = { { 0 } };

cvar_t *auth_server;

#define AUTH_DEFAULT_SERVER ""

#define AUTH_CLIENT_LOGIN "/api/client/login"
#define AUTH_CLIENT_CHALLENGE "/api/client/auth/challenge"
#define AUTH_SERVER_CHALLENGE "/api/server/auth/challenge"
#define AUTH_SERVER_VERIFY "/api/server/auth/verify"

#define A_URL(x) va("%s" x, auth_server->string)

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

	if (cJSON_HasObjectItem(root, "token"))
	{
		authData.authToken[0] = '\0';
		Q_strncpyz(authData.authToken, cJSON_GetStringValue(cJSON_GetObjectItem(root, "token")), AUTH_TOKEN_SIZE);
		Auth_WriteTokenToFile(authData.authToken);
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
	tmp = cJSON_PrintUnformatted(root);
	cJSON_free(root);
	Q_strcpy(upload->contentType, "application/json");
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

static void Auth_ReadToken(void)
{
	long         len = 0;
	fileHandle_t f   = 0;

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

	if (count < 2)
	{
		Com_Printf("Requires arguments\n");
	}

	tmp = Cmd_Argv(1);

	if (!Q_stricmp(tmp, "login"))
	{
		if (*authData.authToken)
		{
			Com_Printf("Already logged in\n");
			return;
		}

		if (count < 4)
		{
			Com_Printf("username and password are required\n");
			return;
		}

		Auth_ClientLogin(Cmd_Argv(2), Cmd_Argv(3));
	}
	else if (!Q_stricmp(tmp, "logout"))
	{
		Auth_ClearToken();
	}
	else
	{
		Com_Printf("Unknown command: %s\n", tmp);
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
		// FIXME: send this to the server
	}

	cJSON_free(root);
}

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
		// FIXME: send this to the client
	}

	cJSON_free(root);
}

static void Auth_ServerVerifyCallback(struct webRequest_s *request, webRequestResult requestResult)
{
	cJSON *root = NULL, *match = NULL;

	Auth_FreeUploadBuffer(request);

	if (requestResult != REQUEST_OK)
	{
		return;
	}

	root = cJSON_Parse((char *)request->data.buffer);

	if (cJSON_HasObjectItem(root, "match"))
	{
		match = cJSON_GetObjectItem(root, "match");
		if (cJSON_IsBool(match) && cJSON_IsTrue(match))
		{
			// FIXME: mark user as logged in..
		}
	}

	cJSON_free(root);
}

void Auth_Server_VerifyResponse(const char *username, const char *challenge, const char *response)
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

	Q_strcpy(upload->contentType, "application/json");
	upload->buffer     = (byte *)json;
	upload->bufferSize = strlen(json);

	Web_CreateRequest(A_URL(AUTH_SERVER_VERIFY), authData.authToken, upload, NULL, &Auth_ServerVerifyCallback, NULL);
}

void Auth_Server_FetchChallenge(const char *username)
{
	cJSON *root;
	char  *json;

	webUploadData_t *upload = NULL;

	upload = Com_Allocate(sizeof(webUploadData_t));
	Com_Memset(upload, 0, sizeof(webUploadData_t));

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "username", username);
	json = cJSON_PrintUnformatted(root);
	cJSON_free(root);

	Q_strcpy(upload->contentType, "application/json");
	upload->buffer     = (byte *)json;
	upload->bufferSize = strlen(json);

	Web_CreateRequest(A_URL(AUTH_SERVER_CHALLENGE), authData.authToken, upload, NULL, &Auth_ServerChallengeCallback, NULL);
}

void Auth_Client_FetchResponse(const char *challenge)
{
	cJSON *root;
	char  *json;

	webUploadData_t *upload = NULL;

	upload = Com_Allocate(sizeof(webUploadData_t));
	Com_Memset(upload, 0, sizeof(webUploadData_t));

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "challenge", challenge);
	json = cJSON_PrintUnformatted(root);
	cJSON_free(root);

	Q_strcpy(upload->contentType, "application/json");
	upload->buffer     = (byte *)json;
	upload->bufferSize = strlen(json);

	Web_CreateRequest(A_URL(AUTH_CLIENT_CHALLENGE), authData.authToken, upload, NULL, &Auth_ClientChallengeCallback, NULL);
}

void Auth_Init(void)
{
	Com_Memset(&authData, 0, sizeof(authData));

	auth_server = Cvar_Get("auth_server", AUTH_DEFAULT_SERVER, CVAR_INIT | CVAR_PROTECTED | CVAR_NOTABCOMPLETE);

	if (!*auth_server->string)
	{
		return;
	}

	Cmd_AddCommand("auth", Auth_Cmd_f, "Authentication handler");

	Auth_ReadToken();
	if (*authData.authToken)
	{
		Auth_TestToken();
	}
}

void Auth_Shutdown(void)
{
	Cmd_RemoveCommand("auth");
}
