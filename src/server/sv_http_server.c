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
 * @file sv_http_server.c
 * @brief Embedded HTTP server implementation
 */

#include "server.h"
#include "sv_http.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#define closesocket close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Global HTTP server state
httpServer_t httpServer; // Non-static for bandwidth throttling access

/**
 * @brief Set socket to non-blocking mode
 * @param[in] sock Socket descriptor
 * @return qtrue on success, qfalse on failure
 */
static qboolean HTTP_SetNonBlocking(int sock)
{
#ifdef _WIN32
	u_long mode = 1;
	if (ioctlsocket(sock, FIONBIO, &mode) != 0)
	{
		Com_Printf("HTTP: Failed to set non-blocking mode: %d\n", WSAGetLastError());
		return qfalse;
	}
#else
	int flags = fcntl(sock, F_GETFL, 0);
	if (flags == -1)
	{
		Com_Printf("HTTP: Failed to get socket flags: %s\n", strerror(errno));
		return qfalse;
	}
	if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		Com_Printf("HTTP: Failed to set non-blocking mode: %s\n", strerror(errno));
		return qfalse;
	}
#endif
	return qtrue;
}

/**
 * @brief Get string representation of socket error
 * @return Error string
 */
static const char *HTTP_GetSocketError(void)
{
#ifdef _WIN32
	static char buf[256];
	int         err = WSAGetLastError();
	Com_sprintf(buf, sizeof(buf), "error %d", err);
	return buf;
#else
	return strerror(errno);
#endif
}

/**
 * @brief Open the HTTP server listening port
 * @param[in] port Port number to listen on
 * @return qtrue on success, qfalse on failure
 */
qboolean HTTP_OpenPort(int port)
{
	struct sockaddr_in address;
	int                sock;
	int                optval = 1;

	if (httpServer.socket != INVALID_SOCKET)
	{
		Com_Printf("HTTP: Server already listening on port %d\n", httpServer.port);
		return qtrue;
	}

	// Create TCP socket
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		Com_Printf("HTTP: Failed to create socket: %s\n", HTTP_GetSocketError());
		return qfalse;
	}

	// Set SO_REUSEADDR to allow binding to same port as UDP socket
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval)) == SOCKET_ERROR)
	{
		Com_Printf("HTTP: Failed to set SO_REUSEADDR: %s\n", HTTP_GetSocketError());
		closesocket(sock);
		return qfalse;
	}

#ifdef SO_REUSEPORT
	// Set SO_REUSEPORT on platforms that support it
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (const char *)&optval, sizeof(optval)) == SOCKET_ERROR)
	{
		Com_DPrintf("HTTP: Failed to set SO_REUSEPORT: %s (not critical)\n", HTTP_GetSocketError());
	}
#endif

	// Bind to port
	memset(&address, 0, sizeof(address));
	address.sin_family      = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port        = htons(port);

	if (bind(sock, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
	{
		Com_Printf("HTTP: Failed to bind to port %d: %s\n", port, HTTP_GetSocketError());
		closesocket(sock);
		return qfalse;
	}

	// Listen with backlog of 16 connections
	if (listen(sock, 16) == SOCKET_ERROR)
	{
		Com_Printf("HTTP: Failed to listen on port %d: %s\n", port, HTTP_GetSocketError());
		closesocket(sock);
		return qfalse;
	}

	// Set non-blocking mode
	if (!HTTP_SetNonBlocking(sock))
	{
		closesocket(sock);
		return qfalse;
	}

	httpServer.socket = sock;
	httpServer.port   = port;

	Com_Printf("HTTP: Server started on port %d\n", port);
	return qtrue;
}

/**
 * @brief Close the HTTP server listening port
 */
void HTTP_ClosePort(void)
{
	if (httpServer.socket != INVALID_SOCKET)
	{
		closesocket(httpServer.socket);
		httpServer.socket = INVALID_SOCKET;
		Com_Printf("HTTP: Server stopped\n");
	}
}

/**
 * @brief Detect server IP address for HTTP URL generation
 * @param[out] ipBuffer Buffer to store IP address string
 * @param[in] ipBufferSize Size of IP buffer
 * @return qtrue on success, qfalse on failure
 */
static qboolean HTTP_DetectServerIP(char *ipBuffer, int ipBufferSize)
{
	cvar_t *net_ip;

	if (!ipBuffer || ipBufferSize < 16)
	{
		return qfalse;
	}

	// First check if net_ip is manually set
	net_ip = Cvar_Get("net_ip", "0.0.0.0", CVAR_LATCH);
	if (net_ip && net_ip->string[0] && Q_stricmp(net_ip->string, "0.0.0.0") != 0 &&
	    Q_stricmp(net_ip->string, "localhost") != 0)
	{
		Q_strncpyz(ipBuffer, net_ip->string, ipBufferSize);
		Com_DPrintf("HTTP: Using net_ip cvar: %s\n", ipBuffer);
		return qtrue;
	}

	// Try to get the external ip
	if (HTTP_GetExternalIP(ipBuffer, ipBufferSize))
	{
		Com_DPrintf("HTTP: Using detected local IP: %s\n", ipBuffer);
		return qtrue;
	}

	// Fallback to 0.0.0.0 which means clients will use their connection IP
	// This is the recommended approach for most server setups as it works
	// correctly even behind NAT or with multiple network interfaces
	Q_strncpyz(ipBuffer, "0.0.0.0", ipBufferSize);
	Com_DPrintf("HTTP: Using fallback IP: 0.0.0.0 (clients will use actual connection IP)\n");
	return qtrue;
}

/**
 * @brief Automatically configure sv_wwwBaseURL for HTTP downloads
 * @return qtrue on success, qfalse on failure
 */
qboolean HTTP_AutoConfigureBaseURL(void)
{
	char   ipAddress[64];
	char   baseURL[256];
	cvar_t *sv_wwwBaseURL;
	cvar_t *sv_wwwDownload;
	int    port;

	if (!httpServer.initialized || !httpServer.enabled)
	{
		Com_DPrintf("HTTP: Cannot auto-configure - server not initialized\n");
		return qfalse;
	}

	// Get sv_wwwBaseURL cvar
	sv_wwwBaseURL = Cvar_Get("sv_wwwBaseURL", "", CVAR_ARCHIVE);

	// If manual URL is set, use it (regardless of auto-config setting)
	if (sv_wwwBaseURL && sv_wwwBaseURL->string[0])
	{
		Com_Printf("HTTP: Using manually configured sv_wwwBaseURL = %s\n", sv_wwwBaseURL->string);
		return qtrue;
	}

	// No manual URL set - check if auto-configuration is enabled
	if (!sv_httpAutoConfig->integer)
	{
		Com_DPrintf("HTTP: Auto-configuration disabled and no manual sv_wwwBaseURL set\n");
		return qfalse;
	}

	// Auto-config enabled and sv_wwwBaseURL is empty - proceed with auto-configuration

	// Detect server IP
	if (!HTTP_DetectServerIP(ipAddress, sizeof(ipAddress)))
	{
		Com_Printf("HTTP: Failed to detect server IP address\n");
		return qfalse;
	}

	// Get server port
	port = httpServer.port;
	if (port <= 0 || port > 65535)
	{
		Com_Printf("HTTP: Invalid port %d\n", port);
		return qfalse;
	}

	// Build base URL
	Com_sprintf(baseURL, sizeof(baseURL), "http://%s:%d", ipAddress, port);

	// Set sv_wwwBaseURL cvar
	Cvar_Set("sv_wwwBaseURL", baseURL);
	Com_Printf("HTTP: Auto-configured sv_wwwBaseURL = %s\n", baseURL);

	// Enable sv_wwwDownload
	sv_wwwDownload = Cvar_Get("sv_wwwDownload", "0", CVAR_ARCHIVE);
	if (sv_wwwDownload && !sv_wwwDownload->integer)
	{
		Cvar_Set("sv_wwwDownload", "1");
		Com_Printf("HTTP: Enabled sv_wwwDownload\n");
	}

	return qtrue;
}

/**
 * @brief Allocate a free client structure
 * @return Pointer to client structure or NULL if none available
 */
httpClient_t *HTTP_AllocClient(void)
{
	int i;

	for (i = 0; i < httpServer.maxClients; i++)
	{
		if (httpServer.clients[i].state == HTTP_STATE_FREE)
		{
			memset(&httpServer.clients[i], 0, sizeof(httpClient_t));
			httpServer.clients[i].state        = HTTP_STATE_CONNECTED;
			httpServer.clients[i].socket       = INVALID_SOCKET;
			httpServer.clients[i].fileHandle   = 0;
			httpServer.clients[i].lastActivity = Sys_Milliseconds();
			httpServer.activeClients++;
			return &httpServer.clients[i];
		}
	}

	return NULL;
}

/**
 * @brief Free a client structure back to the pool
 * @param[in] client Client to free
 */
void HTTP_FreeClient(httpClient_t *client)
{
	if (!client)
	{
		return;
	}

	if (client->state != HTTP_STATE_FREE)
	{
		memset(client, 0, sizeof(httpClient_t));
		client->state  = HTTP_STATE_FREE;
		client->socket = INVALID_SOCKET;
		httpServer.activeClients--;
	}
}

/**
 * @brief Close a client connection
 * @param[in] client Client to close
 */
void HTTP_CloseClient(httpClient_t *client)
{
	if (!client)
	{
		return;
	}

	// Close file if open
	if (client->fileHandle)
	{
		FS_FCloseFile(client->fileHandle);
		client->fileHandle = 0;
	}

	// Close socket
	if (client->socket != INVALID_SOCKET)
	{
		Com_DPrintf("HTTP: Client disconnected: %s\n", NET_AdrToString(&client->address));
		closesocket(client->socket);
		client->socket = INVALID_SOCKET;
	}

	// Free client structure
	HTTP_FreeClient(client);
}

/**
 * @brief Timeout idle client connections
 */
void HTTP_TimeoutClients(void)
{
	int i;
	int currentTime;
	int timeout;

	if (!sv_httpTimeout)
	{
		return;
	}

	timeout = sv_httpTimeout->integer * 1000; // Convert seconds to milliseconds
	if (timeout <= 0)
	{
		return; // Timeout disabled
	}

	currentTime = Sys_Milliseconds();

	for (i = 0; i < httpServer.maxClients; i++)
	{
		httpClient_t *client = &httpServer.clients[i];

		if (client->state == HTTP_STATE_FREE)
		{
			continue;
		}

		// Check if client has been idle too long
		if (currentTime - client->lastActivity > timeout)
		{
			Com_DPrintf("HTTP: Client timeout: %s (idle for %d ms)\n",
			            NET_AdrToString(&client->address),
			            currentTime - client->lastActivity);
			HTTP_CloseClient(client);
		}
	}
}

/**
 * @brief Accept new incoming TCP connections
 */
void HTTP_AcceptConnections(void)
{
	struct sockaddr_in clientAddr;
	socklen_t          addrLen;
	int                clientSock;
	httpClient_t       *client;

	if (httpServer.socket == INVALID_SOCKET)
	{
		return;
	}

	// Accept all pending connections
	while (1)
	{
		addrLen    = sizeof(clientAddr);
		clientSock = accept(httpServer.socket, (struct sockaddr *)&clientAddr, &addrLen);

		if (clientSock == INVALID_SOCKET)
		{
#ifdef _WIN32
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				break; // No more connections
			}
			Com_DPrintf("HTTP: Accept failed: %s\n", HTTP_GetSocketError());
#else
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				break; // No more connections
			}
			Com_DPrintf("HTTP: Accept failed: %s\n", HTTP_GetSocketError());
#endif
			break;
		}

		// Allocate client structure
		client = HTTP_AllocClient();
		if (!client)
		{
			Com_Printf("HTTP: Maximum clients reached, rejecting connection\n");
			closesocket(clientSock);
			continue;
		}

		// Set non-blocking mode
		if (!HTTP_SetNonBlocking(clientSock))
		{
			HTTP_FreeClient(client);
			closesocket(clientSock);
			continue;
		}

		// Initialize client
		client->socket        = clientSock;
		client->address.type  = NA_IP;
		client->address.ip[0] = clientAddr.sin_addr.s_addr & 0xFF;
		client->address.ip[1] = (clientAddr.sin_addr.s_addr >> 8) & 0xFF;
		client->address.ip[2] = (clientAddr.sin_addr.s_addr >> 16) & 0xFF;
		client->address.ip[3] = (clientAddr.sin_addr.s_addr >> 24) & 0xFF;
		client->address.port  = ntohs(clientAddr.sin_port);
		client->state         = HTTP_STATE_CONNECTED;
		client->lastActivity  = Sys_Milliseconds();

		httpServer.totalConnections++;

		Com_Printf("HTTP: Client connected: %s\n", NET_AdrToString(&client->address));
		Com_DPrintf("HTTP: Active clients: %d/%d\n", httpServer.activeClients, httpServer.maxClients);
	}
}

/**
 * @brief Initialize the HTTP server subsystem
 */
void HTTP_Init(void)
{
	int port;

	Com_Printf("------- HTTP_Init -------\n");

	// Initialize server structure
	memset(&httpServer, 0, sizeof(httpServer));
	httpServer.socket = INVALID_SOCKET;

	// CVARs are initialized in sv_init.c

	// Check if HTTP server is enabled
	if (!sv_httpEnable->integer)
	{
		Com_Printf("HTTP: Server disabled\n");
		return;
	}

	// Get port from net_port cvar
	port = Cvar_VariableIntegerValue("net_port");
	if (port <= 0 || port > 65535)
	{
		Com_Printf("HTTP: Invalid port %d\n", port);
		return;
	}

	// Set max clients
	httpServer.maxClients = sv_httpMaxClients->integer;
	if (httpServer.maxClients < 1)
	{
		httpServer.maxClients = 1;
	}
	else if (httpServer.maxClients > MAX_HTTP_CLIENTS)
	{
		httpServer.maxClients = MAX_HTTP_CLIENTS;
	}

	// Open listening port
	if (HTTP_OpenPort(port))
	{
		httpServer.initialized = qtrue;
		httpServer.enabled     = qtrue;

		// Auto-configure base URL if enabled
		HTTP_AutoConfigureBaseURL();
	}
	else
	{
		Com_Printf("HTTP: Failed to initialize server\n");
	}
}

/**
 * @brief Shutdown the HTTP server subsystem
 */
void HTTP_Shutdown(void)
{
	int i;

	if (!httpServer.initialized)
	{
		return;
	}

	Com_Printf("HTTP: Shutting down...\n");

	// Close all client connections
	for (i = 0; i < MAX_HTTP_CLIENTS; i++)
	{
		if (httpServer.clients[i].state != HTTP_STATE_FREE)
		{
			HTTP_CloseClient(&httpServer.clients[i]);
		}
	}

	// Close listening socket
	HTTP_ClosePort();

	// Print statistics
	Com_Printf("HTTP: Total connections: %d\n", httpServer.totalConnections);
	Com_Printf("HTTP: Total bytes sent: %d\n", httpServer.totalBytesSent);
	Com_Printf("HTTP: Total requests: %d\n", httpServer.totalRequests);

	// Clear server structure
	memset(&httpServer, 0, sizeof(httpServer));
	httpServer.socket = INVALID_SOCKET;
}

/**
 * @brief Receive data from a client into request buffer
 * @param[in] client Client to receive data from
 * @return qtrue if data received, qfalse on error or no data
 */
static qboolean HTTP_ReceiveClientData(httpClient_t *client)
{
	int bytesReceived;
	int bufferSpace;

	if (!client || client->socket == INVALID_SOCKET)
	{
		return qfalse;
	}

	// Calculate available buffer space
	bufferSpace = HTTP_REQUEST_BUFFER - client->requestLength - 1;
	if (bufferSpace <= 0)
	{
		Com_DPrintf("HTTP: Request buffer full for client %s\n", NET_AdrToString(&client->address));
		return qfalse;
	}

	// Receive data into buffer
	bytesReceived = recv(client->socket,
	                     client->requestBuffer + client->requestLength,
	                     bufferSpace,
	                     0);

	if (bytesReceived > 0)
	{
		// Data received successfully
		client->requestLength                       += bytesReceived;
		client->requestBuffer[client->requestLength] = '\0'; // Null terminate
		client->lastActivity                         = Sys_Milliseconds();

		Com_DPrintf("HTTP: Received %d bytes from %s (total: %d)\n",
		            bytesReceived, NET_AdrToString(&client->address), client->requestLength);

		return qtrue;
	}
	else if (bytesReceived == 0)
	{
		// Connection closed by client
		Com_DPrintf("HTTP: Connection closed by client %s\n", NET_AdrToString(&client->address));
		return qfalse;
	}
	else
	{
		// Error or would block
#ifdef _WIN32
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			Com_DPrintf("HTTP: Recv error from %s: %s\n",
			            NET_AdrToString(&client->address), HTTP_GetSocketError());
			return qfalse;
		}
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			Com_DPrintf("HTTP: Recv error from %s: %s\n",
			            NET_AdrToString(&client->address), HTTP_GetSocketError());
			return qfalse;
		}
#endif
		// Would block - no data available right now
		return qtrue;
	}
}

/**
 * @brief Check if client request is complete
 * @param[in] client Client to check
 * @return qtrue if request is complete (ends with \r\n\r\n)
 */
static qboolean HTTP_IsRequestComplete(httpClient_t *client)
{
	if (!client)
	{
		return qfalse;
	}

	// Check for end of headers marker
	return (strstr(client->requestBuffer, "\r\n\r\n") != NULL);
}

/**
 * @brief Process client requests
 * @param[in] client Client to process
 */
static void HTTP_ProcessClient(httpClient_t *client)
{
	if (!client)
	{
		return;
	}

	switch (client->state)
	{
	case HTTP_STATE_CONNECTED:
	case HTTP_STATE_RECEIVING:
		// Receive data from client
		if (!HTTP_ReceiveClientData(client))
		{
			// Error or connection closed
			if (client->requestLength == 0 ||
			    client->requestBuffer[0] == '\0' ||
			    recv(client->socket, client->requestBuffer, 1, MSG_PEEK) == 0)
			{
				// Connection closed or error
				HTTP_CloseClient(client);
				return;
			}
			// Check if buffer is full without valid request - prevent buffer exhaustion DoS
			if (client->requestLength >= HTTP_REQUEST_BUFFER - 1 && !HTTP_IsRequestComplete(client))
			{
				Com_DPrintf("HTTP: Request buffer exhausted without valid request from %s\n",
				            NET_AdrToString(&client->address));
				HTTP_SendErrorResponse(client, 400, "Request too large");
				return;
			}
		}

		// Update state
		if (client->requestLength > 0)
		{
			client->state = HTTP_STATE_RECEIVING;
		}

		// Check if request is complete
		if (HTTP_IsRequestComplete(client))
		{
			// Parse the request
			if (HTTP_ParseRequest(client))
			{
				client->state = HTTP_STATE_PROCESSING;
				Com_Printf("HTTP: Request from %s: %s %s\n",
				           NET_AdrToString(&client->address),
				           (client->method == HTTP_METHOD_GET) ? "GET" : "HEAD",
				           client->path);
			}
			else
			{
				// Parse error - send 400 Bad Request
				Com_DPrintf("HTTP: Parse error for client %s\n", NET_AdrToString(&client->address));
				HTTP_SendErrorResponse(client, 400, "Malformed HTTP request");
			}
		}
		break;

	case HTTP_STATE_PROCESSING:
		// Process request and prepare response
		// Initiate file transfer
		HTTP_SendFileResponse(client);
		break;

	case HTTP_STATE_SENDING_HEADERS:
		// Send response headers
		if (!HTTP_ProcessResponseSending(client))
		{
			// Sending complete or error - close connection
			if (client->state != HTTP_STATE_CLOSING)
			{
				client->state = HTTP_STATE_CLOSING;
			}
		}
		// Check if headers are fully sent and we should start sending file
		else if (client->state == HTTP_STATE_SENDING_HEADERS &&
		         client->bytesSent >= client->responseLength &&
		         client->fileHandle)
		{
			// Headers sent, move to file sending
			client->state     = HTTP_STATE_SENDING_FILE;
			client->bytesSent = 0;
			Com_DPrintf("HTTP: Headers complete, starting file transfer\n");
		}
		break;

	case HTTP_STATE_SENDING_FILE:
		// Send file data chunks
		if (!HTTP_SendFileChunk(client))
		{
			// Transfer complete or error - close connection
			if (client->state != HTTP_STATE_CLOSING)
			{
				client->state = HTTP_STATE_CLOSING;
			}
		}
		break;

	case HTTP_STATE_CLOSING:
		HTTP_CloseClient(client);
		break;

	default:
		break;
	}
}

/**
 * @brief Process HTTP server events (called each frame)
 * Implements frame time budget and bandwidth throttling
 */
void HTTP_Frame(void)
{
	int       i;
	int       currentTime;
	int       elapsedTime;
	const int FRAME_TIME_BUDGET_MS = 5; // 5ms budget per frame

	if (!httpServer.initialized || !httpServer.enabled)
	{
		return;
	}

	// Track frame start time and reset frame byte counter
	httpServer.frameStartTime = Sys_Milliseconds();
	httpServer.frameBytesSent = 0;

	// Accept new connections
	HTTP_AcceptConnections();

	// Timeout idle clients
	HTTP_TimeoutClients();

	// Process existing client connections
	for (i = 0; i < httpServer.maxClients; i++)
	{
		if (httpServer.clients[i].state != HTTP_STATE_FREE)
		{
			HTTP_ProcessClient(&httpServer.clients[i]);

			// Check frame time budget
			currentTime = Sys_Milliseconds();
			elapsedTime = currentTime - httpServer.frameStartTime;
			if (elapsedTime >= FRAME_TIME_BUDGET_MS)
			{
				Com_DPrintf("HTTP: Frame time budget exceeded (%d ms), deferring remaining clients\n", elapsedTime);
				break; // Continue next frame
			}
		}
	}
}
