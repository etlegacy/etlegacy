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
 * @file sv_http_response.c
 * @brief HTTP response generation and transmission
 */

#include "server.h"
#include "sv_http.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <errno.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// External access to server state for bandwidth throttling
extern httpServer_t httpServer;
extern cvar_t       *sv_httpMaxBytesPerFrame;

/**
 * @brief Get HTTP status text for status code
 * @param[in] statusCode HTTP status code
 * @return Status text string
 */
const char *HTTP_GetStatusText(int statusCode)
{
	switch (statusCode)
	{
	case 200:
		return "OK";
	case 206:
		return "Partial Content";
	case 400:
		return "Bad Request";
	case 404:
		return "Not Found";
	case 416:
		return "Range Not Satisfiable";
	case 500:
		return "Internal Server Error";
	case 503:
		return "Service Unavailable";
	default:
		return "Unknown";
	}
}

/**
 * @brief Determine MIME type from file extension
 * @param[in] path File path
 * @return MIME type string
 */
const char *HTTP_GetMimeType(const char *path)
{
	const char *ext;

	if (!path)
	{
		return "application/octet-stream";
	}

	// Find file extension
	ext = strrchr(path, '.');
	if (!ext)
	{
		return "application/octet-stream";
	}

	ext++; // Skip the dot

	// Check known extensions
	if (!Q_stricmp(ext, "pk3"))
	{
		return "application/octet-stream";
	}
	else if (!Q_stricmp(ext, "txt"))
	{
		return "text/plain";
	}
	else if (!Q_stricmp(ext, "html") || !Q_stricmp(ext, "htm"))
	{
		return "text/html";
	}

	// Default to binary
	return "application/octet-stream";
}

/**
 * @brief Build response headers for a given status code
 * @param[in] client Client to build headers for
 * @param[in] statusCode HTTP status code
 * @param[in] contentType Content-Type header value
 * @param[in] contentLength Content-Length value
 * @return qtrue on success, qfalse on error
 */
qboolean HTTP_BuildResponseHeaders(httpClient_t *client, int statusCode, const char *contentType, int contentLength)
{
	int len;

	if (!client)
	{
		return qfalse;
	}

	// Start with status line
	len = Com_sprintf(client->responseBuffer, sizeof(client->responseBuffer),
	                  "HTTP/1.1 %d %s\r\n", statusCode, HTTP_GetStatusText(statusCode));

	// Add Server header
	len += Com_sprintf(client->responseBuffer + len, sizeof(client->responseBuffer) - len,
	                   "Server: ETLegacy/%s\r\n", Q3_VERSION);

	// Add Content-Type header
	if (contentType && contentType[0])
	{
		len += Com_sprintf(client->responseBuffer + len, sizeof(client->responseBuffer) - len,
		                   "Content-Type: %s\r\n", contentType);
	}

	// Add Content-Length header
	if (contentLength >= 0)
	{
		len += Com_sprintf(client->responseBuffer + len, sizeof(client->responseBuffer) - len,
		                   "Content-Length: %d\r\n", contentLength);
	}

	// Add Accept-Ranges header for file responses
	if (statusCode == 200 || statusCode == 206)
	{
		len += Com_sprintf(client->responseBuffer + len, sizeof(client->responseBuffer) - len,
		                   "Accept-Ranges: bytes\r\n");
	}

	// Add Content-Range header for partial content
	if (statusCode == 206 && client->hasRange)
	{
		int rangeEnd = client->rangeEnd;
		if (rangeEnd < 0 || rangeEnd >= client->fileSize)
		{
			rangeEnd = client->fileSize - 1;
		}

		len += Com_sprintf(client->responseBuffer + len, sizeof(client->responseBuffer) - len,
		                   "Content-Range: bytes %d-%d/%d\r\n",
		                   client->rangeStart, rangeEnd, client->fileSize);
	}

	// Add Connection header (always close for now)
	len += Com_sprintf(client->responseBuffer + len, sizeof(client->responseBuffer) - len,
	                   "Connection: close\r\n");

	// End headers with blank line
	len += Com_sprintf(client->responseBuffer + len, sizeof(client->responseBuffer) - len, "\r\n");

	// Check for overflow
	if (len >= sizeof(client->responseBuffer))
	{
		Com_Printf("HTTP: Response header buffer overflow\n");
		return qfalse;
	}

	client->responseLength = len;

	Com_DPrintf("HTTP: Built response headers (%d bytes):\n%s", len, client->responseBuffer);

	return qtrue;
}

/**
 * @brief Send response headers to client
 * @param[in] client Client to send to
 * @return qtrue if headers fully sent, qfalse if would block or error
 * Implements bandwidth throttling
 */
qboolean HTTP_SendHeaders(httpClient_t *client)
{
	int bytesSent;
	int bytesToSend;
	int maxBytesPerFrame;
	int bytesAvailableThisFrame;

	if (!client || client->socket == INVALID_SOCKET)
	{
		return qfalse;
	}

	// Calculate how much to send
	bytesToSend = client->responseLength - client->bytesSent;
	if (bytesToSend <= 0)
	{
		// Headers already sent
		return qtrue;
	}

	// Check bandwidth throttling
	if (sv_httpMaxBytesPerFrame && sv_httpMaxBytesPerFrame->integer > 0)
	{
		maxBytesPerFrame        = sv_httpMaxBytesPerFrame->integer;
		bytesAvailableThisFrame = maxBytesPerFrame - httpServer.frameBytesSent;

		if (bytesAvailableThisFrame <= 0)
		{
			// Bandwidth limit reached for this frame, defer to next frame
			Com_DPrintf("HTTP: Bandwidth limit reached, deferring header send\n");
			return qtrue; // Return true to avoid closing connection
		}

		// Limit bytes to send this frame
		if (bytesToSend > bytesAvailableThisFrame)
		{
			bytesToSend = bytesAvailableThisFrame;
		}
	}

	// Send data
	bytesSent = send(client->socket,
	                 client->responseBuffer + client->bytesSent,
	                 bytesToSend,
	                 0);

	if (bytesSent > 0)
	{
		client->bytesSent         += bytesSent;
		client->lastActivity       = Sys_Milliseconds();
		httpServer.frameBytesSent += bytesSent;      // Track frame bytes
		httpServer.totalBytesSent += bytesSent;       // Update total stats

		Com_DPrintf("HTTP: Sent %d header bytes to %s (%d/%d)\n",
		            bytesSent, NET_AdrToString(&client->address),
		            client->bytesSent, client->responseLength);

		// Check if all headers sent
		if (client->bytesSent >= client->responseLength)
		{
			Com_DPrintf("HTTP: Headers fully sent to %s\n", NET_AdrToString(&client->address));
			return qtrue;
		}

		// More to send, but return true to continue
		return qtrue;
	}
	else if (bytesSent == 0)
	{
		// Connection closed
		Com_DPrintf("HTTP: Connection closed while sending headers to %s\n",
		            NET_AdrToString(&client->address));
		return qfalse;
	}
	else
	{
		// Error or would block
#ifdef _WIN32
		int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
		{
			// Would block - try again next frame
			return qtrue;
		}
		Com_DPrintf("HTTP: Send error to %s: %d\n", NET_AdrToString(&client->address), err);
#else
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			// Would block - try again next frame
			return qtrue;
		}
		Com_DPrintf("HTTP: Send error to %s: %s\n", NET_AdrToString(&client->address), strerror(errno));
#endif
		return qfalse;
	}
}

/**
 * @brief Send error response to client
 * @param[in] client Client to send error to
 * @param[in] statusCode HTTP status code
 * @param[in] message Error message
 */
void HTTP_SendErrorResponse(httpClient_t *client, int statusCode, const char *message)
{
	char body[1024];
	int  bodyLen;

	if (!client)
	{
		return;
	}

	// Build error body
	bodyLen = Com_sprintf(body, sizeof(body),
	                      "<html><head><title>%d %s</title></head>\n"
	                      "<body><h1>%d %s</h1>\n"
	                      "<p>%s</p>\n"
	                      "<hr><p>ETLegacy/%s</p>\n"
	                      "</body></html>\n",
	                      statusCode, HTTP_GetStatusText(statusCode),
	                      statusCode, HTTP_GetStatusText(statusCode),
	                      message ? message : HTTP_GetStatusText(statusCode),
	                      Q3_VERSION);

	// Build response headers
	if (!HTTP_BuildResponseHeaders(client, statusCode, "text/html", bodyLen))
	{
		HTTP_CloseClient(client);
		return;
	}

	// Append body to response buffer if there's space
	if (client->responseLength + bodyLen <= sizeof(client->responseBuffer))
	{
		memcpy(client->responseBuffer + client->responseLength, body, bodyLen);
		client->responseLength += bodyLen;
	}

	// Reset send counter
	client->bytesSent = 0;
	client->state     = HTTP_STATE_SENDING_HEADERS;

	Com_DPrintf("HTTP: Sending error response %d to %s: %s\n",
	            statusCode, NET_AdrToString(&client->address), message ? message : "");

	// Try to send immediately
	if (HTTP_SendHeaders(client))
	{
		if (client->bytesSent >= client->responseLength)
		{
			// All sent, close connection
			client->state = HTTP_STATE_CLOSING;
		}
	}
	else
	{
		// Send failed, close connection
		client->state = HTTP_STATE_CLOSING;
	}
}

/**
 * @brief Process response sending state (headers only or complete response)
 * @param[in] client Client to process
 * @return qtrue if processing should continue, qfalse if done or error
 */
qboolean HTTP_ProcessResponseSending(httpClient_t *client)
{
	if (!client)
	{
		return qfalse;
	}

	if (client->state != HTTP_STATE_SENDING_HEADERS)
	{
		return qtrue;
	}

	// Send headers
	if (!HTTP_SendHeaders(client))
	{
		// Error sending headers
		client->state = HTTP_STATE_CLOSING;
		return qfalse;
	}

	// Check if all data sent
	if (client->bytesSent >= client->responseLength)
	{
		// All sent, close connection (for now, no keep-alive)
		Com_DPrintf("HTTP: Response fully sent to %s\n", NET_AdrToString(&client->address));
		client->state = HTTP_STATE_CLOSING;
		return qfalse;
	}

	// Still sending, continue next frame
	return qtrue;
}

/**
 * @brief Send file response to client (initiate file transfer)
 * @param[in] client Client to send file to
 * @return qtrue on success, qfalse on error
 */
qboolean HTTP_SendFileResponse(httpClient_t *client)
{
	int        statusCode;
	int        contentLength;
	const char *mimeType;

	if (!client)
	{
		return qfalse;
	}

	// Validate path for security
	if (!HTTP_ValidatePath(client->path))
	{
		HTTP_SendErrorResponse(client, 400, "Invalid file path");
		return qfalse;
	}

	// Open file using secure filesystem (validates existence, opens file, gets size)
	if (!HTTP_OpenFileForServing(client->path, &client->fileHandle, &client->fileSize))
	{
		HTTP_SendErrorResponse(client, 404, "File not found");
		return qfalse;
	}

	// Handle range requests
	if (client->hasRange)
	{
		// Validate range
		if (client->rangeStart < 0 || client->rangeStart >= client->fileSize)
		{
			HTTP_CloseFile(client->fileHandle);
			client->fileHandle = 0;
			HTTP_SendErrorResponse(client, 416, "Range not satisfiable");
			return qfalse;
		}

		// Adjust range end if needed
		if (client->rangeEnd < 0 || client->rangeEnd >= client->fileSize)
		{
			client->rangeEnd = client->fileSize - 1;
		}

		// Seek to start position
		if (!HTTP_SeekFile(client->fileHandle, client->rangeStart, FS_SEEK_SET))
		{
			HTTP_CloseFile(client->fileHandle);
			client->fileHandle = 0;
			HTTP_SendErrorResponse(client, 500, "Failed to seek file");
			return qfalse;
		}

		// Calculate bytes to send
		contentLength           = client->rangeEnd - client->rangeStart + 1;
		client->fileBytesToSend = contentLength;
		client->filePosition    = client->rangeStart;
		statusCode              = 206; // Partial Content

		Com_Printf("HTTP: Sending partial file: %s (bytes %d-%d/%d) to %s\n",
		           client->path, client->rangeStart, client->rangeEnd,
		           client->fileSize, NET_AdrToString(&client->address));
	}
	else
	{
		// Send entire file
		contentLength           = client->fileSize;
		client->fileBytesToSend = client->fileSize;
		client->filePosition    = 0;
		statusCode              = 200; // OK

		Com_Printf("HTTP: Sending file: %s (%d bytes) to %s\n",
		           client->path, client->fileSize, NET_AdrToString(&client->address));
	}

	// Determine MIME type
	mimeType = HTTP_GetMimeType(client->path);

	// Build response headers
	if (!HTTP_BuildResponseHeaders(client, statusCode, mimeType, contentLength))
	{
		HTTP_CloseFile(client->fileHandle);
		client->fileHandle = 0;
		return qfalse;
	}

	// Reset counters
	client->bytesSent = 0;
	client->state     = HTTP_STATE_SENDING_HEADERS;

	// Try to send headers immediately
	if (HTTP_SendHeaders(client))
	{
		// Check if headers fully sent
		if (client->bytesSent >= client->responseLength)
		{
			// Headers sent, move to file sending state
			client->state     = HTTP_STATE_SENDING_FILE;
			client->bytesSent = 0; // Reset for file data
			Com_DPrintf("HTTP: Headers sent, beginning file transfer\n");
		}
	}
	else
	{
		// Send failed, close
		HTTP_CloseFile(client->fileHandle);
		client->fileHandle = 0;
		client->state      = HTTP_STATE_CLOSING;
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Send file data chunk to client
 * @param[in] client Client to send chunk to
 * @return qtrue if more data to send, qfalse if complete or error
 * Implements bandwidth throttling
 */
qboolean HTTP_SendFileChunk(httpClient_t *client)
{
	static char fileChunkBuffer[HTTP_FILE_CHUNK];
	int         bytesToRead;
	int         bytesRead;
	int         bytesSent;
	int         totalSent;
	int         maxBytesPerFrame;
	int         bytesAvailableThisFrame;

	if (!client || !client->fileHandle)
	{
		return qfalse;
	}

	// Check bandwidth throttling before reading
	if (sv_httpMaxBytesPerFrame && sv_httpMaxBytesPerFrame->integer > 0)
	{
		maxBytesPerFrame        = sv_httpMaxBytesPerFrame->integer;
		bytesAvailableThisFrame = maxBytesPerFrame - httpServer.frameBytesSent;

		if (bytesAvailableThisFrame <= 0)
		{
			// Bandwidth limit reached for this frame, defer to next frame
			Com_DPrintf("HTTP: Bandwidth limit reached, deferring file chunk send\n");
			return qtrue; // Return true to continue next frame
		}
	}
	else
	{
		bytesAvailableThisFrame = HTTP_FILE_CHUNK; // No limit
	}

	// Calculate how much to read this chunk
	bytesToRead = client->fileBytesToSend;
	if (bytesToRead > HTTP_FILE_CHUNK)
	{
		bytesToRead = HTTP_FILE_CHUNK;
	}

	// Limit by available bandwidth this frame
	if (bytesToRead > bytesAvailableThisFrame)
	{
		bytesToRead = bytesAvailableThisFrame;
	}

	if (bytesToRead <= 0)
	{
		// No more data to send
		Com_Printf("HTTP: File transfer complete: %d bytes sent to %s\n",
		           client->filePosition, NET_AdrToString(&client->address));
		HTTP_CloseFile(client->fileHandle);
		client->fileHandle = 0;
		client->state      = HTTP_STATE_CLOSING;
		return qfalse;
	}

	// Read chunk from file
	bytesRead = HTTP_ReadFileChunk(client->fileHandle, fileChunkBuffer, bytesToRead);
	if (bytesRead <= 0)
	{
		// Read error or EOF
		Com_DPrintf("HTTP: File read error or EOF\n");
		HTTP_CloseFile(client->fileHandle);
		client->fileHandle = 0;
		client->state      = HTTP_STATE_CLOSING;
		return qfalse;
	}

	// Send chunk to client
	totalSent = 0;
	while (totalSent < bytesRead)
	{
		bytesSent = send(client->socket,
		                 fileChunkBuffer + totalSent,
		                 bytesRead - totalSent,
		                 0);

		if (bytesSent > 0)
		{
			totalSent           += bytesSent;
			client->lastActivity = Sys_Milliseconds();
		}
		else if (bytesSent == 0)
		{
			// Connection closed
			Com_DPrintf("HTTP: Connection closed while sending file\n");
			HTTP_CloseFile(client->fileHandle);
			client->fileHandle = 0;
			client->state      = HTTP_STATE_CLOSING;
			return qfalse;
		}
		else
		{
			// Error or would block
#ifdef _WIN32
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				// Would block - continue next frame
				// Seek back to where we were
				if (totalSent < bytesRead)
				{
					int seekBack = -(bytesRead - totalSent);
					FS_Seek(client->fileHandle, seekBack, FS_SEEK_CUR);
				}
				return qtrue;
			}
			Com_DPrintf("HTTP: Send error: %d\n", err);
#else
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				// Would block - continue next frame
				// Seek back to where we were
				if (totalSent < bytesRead)
				{
					int seekBack = -(bytesRead - totalSent);
					FS_Seek(client->fileHandle, seekBack, FS_SEEK_CUR);
				}
				return qtrue;
			}
			Com_DPrintf("HTTP: Send error: %s\n", strerror(errno));
#endif
			HTTP_CloseFile(client->fileHandle);
			client->fileHandle = 0;
			client->state      = HTTP_STATE_CLOSING;
			return qfalse;
		}
	}

	// Update counters
	client->filePosition      += totalSent;
	client->fileBytesToSend   -= totalSent;
	client->bytesSent         += totalSent;
	httpServer.frameBytesSent += totalSent;  // Track frame bytes
	httpServer.totalBytesSent += totalSent;   // Update total stats

	Com_DPrintf("HTTP: Sent %d bytes to %s (total: %d, remaining: %d)\n",
	            totalSent, NET_AdrToString(&client->address),
	            client->filePosition, client->fileBytesToSend);

	// Check if we're done
	if (client->fileBytesToSend <= 0)
	{
		Com_Printf("HTTP: File transfer complete: %d bytes sent to %s\n",
		           client->filePosition, NET_AdrToString(&client->address));
		HTTP_CloseFile(client->fileHandle);
		client->fileHandle = 0;
		client->state      = HTTP_STATE_CLOSING;
		return qfalse;
	}

	// More data to send next frame
	return qtrue;
}
