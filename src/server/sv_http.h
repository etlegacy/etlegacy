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
 * @file sv_http.h
 * @brief Embedded HTTP server for serving game files
 */

#ifndef INCLUDE_SV_HTTP_H
#define INCLUDE_SV_HTTP_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

// Maximum number of simultaneous HTTP client connections
#define MAX_HTTP_CLIENTS        32

// Buffer sizes
#define HTTP_REQUEST_BUFFER     16384   // 16KB for request headers
#define HTTP_RESPONSE_BUFFER    4096    // 4KB for response headers
#define HTTP_FILE_CHUNK         32768   // 32KB for file chunks

// HTTP request method
typedef enum
{
	HTTP_METHOD_NONE,
	HTTP_METHOD_GET,
	HTTP_METHOD_HEAD
} httpMethod_t;

// HTTP client connection state
typedef enum
{
	HTTP_STATE_FREE,            // Client slot is free
	HTTP_STATE_CONNECTED,       // Client connected, waiting for request
	HTTP_STATE_RECEIVING,       // Receiving request data
	HTTP_STATE_PROCESSING,      // Processing request
	HTTP_STATE_SENDING_HEADERS, // Sending response headers
	HTTP_STATE_SENDING_FILE,    // Sending file data
	HTTP_STATE_CLOSING          // Closing connection
} httpState_t;

/**
 * @struct httpClient_s
 * @typedef httpClient_t
 * @brief HTTP client connection structure
 */
typedef struct httpClient_s
{
	httpState_t state;          // Connection state
	int socket;                 // Client socket descriptor
	netadr_t address;           // Client address
	int lastActivity;           // Time of last activity (msec)

	// Request data
	char requestBuffer[HTTP_REQUEST_BUFFER];    // Request buffer
	int requestLength;                          // Current request length
	httpMethod_t method;                        // HTTP method
	char path[MAX_QPATH];                       // Requested path
	char httpVersion[16];                       // HTTP version string

	// Request headers
	char userAgent[256];                        // User-Agent header
	char referer[256];                          // Referer header

	// Range request support
	qboolean hasRange;                          // Has Range header
	int rangeStart;                             // Range start position
	int rangeEnd;                               // Range end position

	// Response data
	char responseBuffer[HTTP_RESPONSE_BUFFER]; // Response header buffer
	int responseLength;                         // Response header length
	int bytesSent;                              // Bytes sent so far

	// File serving
	fileHandle_t fileHandle;                    // Open file handle
	int fileSize;                               // Total file size
	int filePosition;                           // Current position in file
	int fileBytesToSend;                        // Bytes remaining to send
} httpClient_t;

/**
 * @struct httpServer_s
 * @typedef httpServer_t
 * @brief HTTP server state structure
 */
typedef struct httpServer_s
{
	qboolean initialized;       // Server initialized
	qboolean enabled;           // Server enabled
	int socket;                 // Listening socket descriptor
	int port;                   // Listening port
	int maxClients;             // Maximum clients

	httpClient_t clients[MAX_HTTP_CLIENTS]; // Client pool
	int activeClients;          // Number of active clients

	// Performance tracking
	int frameBytesSent;         // Bytes sent in current frame
	int frameStartTime;         // Frame start time (msec)

	// Statistics
	int totalConnections;       // Total connections accepted
	int totalBytesSent;         // Total bytes sent
	int totalRequests;          // Total requests processed
} httpServer_t;

// Public API

/**
 * @brief Get the external IP address of the server
 * @param[out] ipBuffer Buffer to store the IP address string
 * @param[in] ipBufferSize Size of the IP address buffer
 */
qboolean HTTP_GetExternalIP(char *ipBuffer, int ipBufferSize);

/**
 * @brief Initialize the HTTP server subsystem
 */
void HTTP_Init(void);

/**
 * @brief Shutdown the HTTP server subsystem
 */
void HTTP_Shutdown(void);

/**
 * @brief Process HTTP server events (called each frame)
 */
void HTTP_Frame(void);

/**
 * @brief Open the HTTP server listening port
 * @param[in] port Port number to listen on
 * @return qtrue on success, qfalse on failure
 */
qboolean HTTP_OpenPort(int port);

/**
 * @brief Close the HTTP server listening port
 */
void HTTP_ClosePort(void);

/**
 * @brief Automatically configure sv_wwwBaseURL for HTTP downloads
 * @return qtrue on success, qfalse on failure
 */
qboolean HTTP_AutoConfigureBaseURL(void);

/**
 * @brief Accept new incoming TCP connections
 */
void HTTP_AcceptConnections(void);

/**
 * @brief Allocate a free client structure
 * @return Pointer to client structure or NULL if none available
 */
httpClient_t *HTTP_AllocClient(void);

/**
 * @brief Free a client structure back to the pool
 * @param[in] client Client to free
 */
void HTTP_FreeClient(httpClient_t *client);

/**
 * @brief Close a client connection
 * @param[in] client Client to close
 */
void HTTP_CloseClient(httpClient_t *client);

/**
 * @brief Timeout idle client connections
 */
void HTTP_TimeoutClients(void);

// Request parsing functions

/**
 * @brief Parse HTTP request from client buffer
 * @param[in] client Client to parse request for
 * @return qtrue on success, qfalse on parse error
 */
qboolean HTTP_ParseRequest(httpClient_t *client);

/**
 * @brief Parse HTTP request line (method, path, version)
 * @param[in] client Client to parse for
 * @param[in] line Request line string
 * @return qtrue on success, qfalse on parse error
 */
qboolean HTTP_ParseRequestLine(httpClient_t *client, const char *line);

/**
 * @brief Parse HTTP headers from request
 * @param[in] client Client to parse for
 * @param[in] headers Headers string (after request line)
 * @return qtrue on success, qfalse on parse error
 */
qboolean HTTP_ParseHeaders(httpClient_t *client, const char *headers);

/**
 * @brief Parse Range header for partial content requests
 * @param[in] client Client to parse for
 * @param[in] value Range header value
 * @return qtrue on success, qfalse on parse error
 */
qboolean HTTP_ParseRangeHeader(httpClient_t *client, const char *value);

// Response generation functions

/**
 * @brief Get HTTP status text for status code
 * @param[in] statusCode HTTP status code
 * @return Status text string
 */
const char *HTTP_GetStatusText(int statusCode);

/**
 * @brief Determine MIME type from file extension
 * @param[in] path File path
 * @return MIME type string
 */
const char *HTTP_GetMimeType(const char *path);

/**
 * @brief Build response headers for a given status code
 * @param[in] client Client to build headers for
 * @param[in] statusCode HTTP status code
 * @param[in] contentType Content-Type header value
 * @param[in] contentLength Content-Length value (-1 for no Content-Length)
 * @return qtrue on success, qfalse on error
 */
qboolean HTTP_BuildResponseHeaders(httpClient_t *client, int statusCode, const char *contentType, int contentLength);

/**
 * @brief Send response headers to client
 * @param[in] client Client to send to
 * @return qtrue if headers sent or in progress, qfalse on error
 */
qboolean HTTP_SendHeaders(httpClient_t *client);

/**
 * @brief Send error response to client
 * @param[in] client Client to send error to
 * @param[in] statusCode HTTP status code
 * @param[in] message Error message
 */
void HTTP_SendErrorResponse(httpClient_t *client, int statusCode, const char *message);

/**
 * @brief Process response sending state
 * @param[in] client Client to process
 * @return qtrue if processing should continue, qfalse if done or error
 */
qboolean HTTP_ProcessResponseSending(httpClient_t *client);

// File operations functions

/**
 * @brief Validate requested path for security
 * @param[in] path Requested path from HTTP request
 * @return qtrue if path is valid and safe, qfalse otherwise
 */
qboolean HTTP_ValidatePath(const char *path);

/**
 * @brief Open file for serving over HTTP using secure filesystem
 * @param[in] path Game-relative path to file (e.g., "etmain/pak0.pk3")
 * @param[out] fileHandle Pointer to store file handle
 * @param[out] fileSize Pointer to store file size
 * @return qtrue if file opened successfully, qfalse otherwise
 */
qboolean HTTP_OpenFileForServing(const char *path, fileHandle_t *fileHandle, int *fileSize);

/**
 * @brief Close open file
 * @param[in] fileHandle File handle to close
 */
void HTTP_CloseFile(fileHandle_t fileHandle);

/**
 * @brief Read chunk of data from file
 * @param[in] fileHandle Open file handle
 * @param[out] buffer Buffer to read data into
 * @param[in] bufferSize Size of buffer
 * @return Number of bytes read, or -1 on error
 */
int HTTP_ReadFileChunk(fileHandle_t fileHandle, void *buffer, int bufferSize);

/**
 * @brief Seek to position in file
 * @param[in] fileHandle Open file handle
 * @param[in] offset Offset to seek to
 * @param[in] whence Seek mode (FS_SEEK_SET, FS_SEEK_CUR, FS_SEEK_END)
 * @return qtrue on success, qfalse on error
 */
qboolean HTTP_SeekFile(fileHandle_t fileHandle, int offset, int whence);

/**
 * @brief Send file response to client (initiate file transfer)
 * @param[in] client Client to send file to
 * @return qtrue on success, qfalse on error
 */
qboolean HTTP_SendFileResponse(httpClient_t *client);

/**
 * @brief Send file data chunk to client
 * @param[in] client Client to send chunk to
 * @return qtrue if more data to send, qfalse if complete or error
 */
qboolean HTTP_SendFileChunk(httpClient_t *client);

#endif // INCLUDE_SV_HTTP_H
