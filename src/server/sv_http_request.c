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
 * @file sv_http_request.c
 * @brief HTTP request parsing implementation
 */

#include "server.h"
#include "sv_http.h"

/**
 * @brief Parse HTTP request line (method, path, version)
 * @param[in] client Client to parse for
 * @param[in] line Request line string
 * @return qtrue on success, qfalse on parse error
 */
qboolean HTTP_ParseRequestLine(httpClient_t *client, const char *line)
{
	char method[16];
	char path[MAX_QPATH];
	char version[16];
	int  result;

	if (!client || !line)
	{
		return qfalse;
	}

	// Parse request line: "METHOD /path HTTP/version"
	result = sscanf(line, "%15s %255s %15s", method, path, version);
	if (result != 3)
	{
		Com_DPrintf("HTTP: Malformed request line: %s\n", line);
		return qfalse;
	}

	// Parse HTTP method
	if (!Q_stricmp(method, "GET"))
	{
		client->method = HTTP_METHOD_GET;
	}
	else if (!Q_stricmp(method, "HEAD"))
	{
		client->method = HTTP_METHOD_HEAD;
	}
	else
	{
		Com_DPrintf("HTTP: Unsupported method: %s\n", method);
		return qfalse;
	}

	// Validate HTTP version (accept HTTP/1.0 and HTTP/1.1)
	if (Q_stricmp(version, "HTTP/1.0") && Q_stricmp(version, "HTTP/1.1"))
	{
		Com_DPrintf("HTTP: Unsupported HTTP version: %s\n", version);
		return qfalse;
	}

	// Copy parsed values
	Q_strncpyz(client->path, path, sizeof(client->path));
	Q_strncpyz(client->httpVersion, version, sizeof(client->httpVersion));

	Com_DPrintf("HTTP: Request: %s %s %s\n", method, client->path, client->httpVersion);

	return qtrue;
}

/**
 * @brief Parse Range header for partial content requests
 * @param[in] client Client to parse for
 * @param[in] value Range header value (e.g., "bytes=1024-2047")
 * @return qtrue on success, qfalse on parse error
 */
qboolean HTTP_ParseRangeHeader(httpClient_t *client, const char *value)
{
	int start = -1;
	int end   = -1;
	int result;

	if (!client || !value)
	{
		return qfalse;
	}

	// Parse "bytes=start-end" format
	if (Q_stricmpn(value, "bytes=", 6) != 0)
	{
		Com_DPrintf("HTTP: Invalid Range header format: %s\n", value);
		return qfalse;
	}

	value += 6; // Skip "bytes="

	// Try to parse both start and end
	result = sscanf(value, "%d-%d", &start, &end);

	if (result == 2)
	{
		// Both start and end specified: "bytes=1024-2047"
		if (start < 0 || end < start)
		{
			Com_DPrintf("HTTP: Invalid range: %d-%d\n", start, end);
			return qfalse;
		}
		client->hasRange   = qtrue;
		client->rangeStart = start;
		client->rangeEnd   = end;
	}
	else if (result == 1 && start >= 0)
	{
		// Only start specified: "bytes=1024-" (open-ended)
		client->hasRange   = qtrue;
		client->rangeStart = start;
		client->rangeEnd   = -1; // Will be set to file size later
	}
	else
	{
		Com_DPrintf("HTTP: Failed to parse range: %s\n", value);
		return qfalse;
	}

	Com_DPrintf("HTTP: Range request: bytes=%d-%d\n", client->rangeStart, client->rangeEnd);

	return qtrue;
}

/**
 * @brief Parse HTTP headers from request
 * @param[in] client Client to parse for
 * @param[in] headers Headers string (after request line)
 * @return qtrue on success, qfalse on parse error
 */
qboolean HTTP_ParseHeaders(httpClient_t *client, const char *headers)
{
	const char *line;
	const char *next;
	char       headerLine[1024];
	char       headerName[256];
	char       headerValue[768];
	qboolean   hasHost = qfalse;
	size_t     lineLen;
	const char *colon;
	size_t     nameLen;
	const char *valueStart;

	if (!client || !headers)
	{
		return qfalse;
	}

	// Initialize default values
	client->hasRange = qfalse;

	// Parse each header line
	line = headers;
	while (*line)
	{
		// Find end of line
		next = strstr(line, "\r\n");
		if (!next)
		{
			break; // No more lines
		}

		// Copy line to buffer
		lineLen = next - line;
		if (lineLen >= sizeof(headerLine))
		{
			lineLen = sizeof(headerLine) - 1;
		}
		Q_strncpyz(headerLine, line, lineLen + 1);

		// Skip empty lines (end of headers)
		if (headerLine[0] == '\0')
		{
			break;
		}

		// Parse header: "Name: Value"
		colon = strchr(headerLine, ':');
		if (!colon)
		{
			// Skip malformed header
			line = next + 2;
			continue;
		}

		// Extract header name
		nameLen = colon - headerLine;
		if (nameLen >= sizeof(headerName))
		{
			nameLen = sizeof(headerName) - 1;
		}
		Q_strncpyz(headerName, headerLine, nameLen + 1);

		// Extract header value (skip leading whitespace)
		valueStart = colon + 1;
		while (*valueStart == ' ' || *valueStart == '\t')
		{
			valueStart++;
		}
		Q_strncpyz(headerValue, valueStart, sizeof(headerValue));

		// Process known headers
		if (!Q_stricmp(headerName, "Host"))
		{
			hasHost = qtrue;
			Com_DPrintf("HTTP: Host: %s\n", headerValue);
		}
		else if (!Q_stricmp(headerName, "User-Agent"))
		{
			Q_strncpyz(client->userAgent, headerValue, sizeof(client->userAgent));
			Com_DPrintf("HTTP: User-Agent: %s\n", client->userAgent);
		}
		else if (!Q_stricmp(headerName, "Referer"))
		{
			Q_strncpyz(client->referer, headerValue, sizeof(client->referer));
			Com_DPrintf("HTTP: Referer: %s\n", client->referer);
		}
		else if (!Q_stricmp(headerName, "Range"))
		{
			if (!HTTP_ParseRangeHeader(client, headerValue))
			{
				Com_DPrintf("HTTP: Failed to parse Range header\n");
			}
		}

		// Move to next line
		line = next + 2;
	}

	// HTTP/1.1 requires Host header
	if (!Q_stricmp(client->httpVersion, "HTTP/1.1") && !hasHost)
	{
		Com_DPrintf("HTTP: Missing required Host header for HTTP/1.1\n");
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Parse HTTP request from client buffer
 * @param[in] client Client to parse request for
 * @return qtrue on success, qfalse on parse error
 */
qboolean HTTP_ParseRequest(httpClient_t *client)
{
	const char *requestEnd;
	const char *headersStart;
	char       requestLine[1024];
	size_t     lineLen;

	if (!client)
	{
		return qfalse;
	}

	// Check if request is complete (ends with \r\n\r\n)
	requestEnd = strstr(client->requestBuffer, "\r\n\r\n");
	if (!requestEnd)
	{
		// Request not complete yet
		return qfalse;
	}

	// Find end of first line (request line)
	headersStart = strstr(client->requestBuffer, "\r\n");
	if (!headersStart)
	{
		Com_DPrintf("HTTP: Malformed request (no request line)\n");
		return qfalse;
	}

	// Extract request line
	lineLen = headersStart - client->requestBuffer;
	if (lineLen >= sizeof(requestLine))
	{
		Com_DPrintf("HTTP: Request line too long\n");
		return qfalse;
	}
	Q_strncpyz(requestLine, client->requestBuffer, lineLen + 1);

	// Parse request line
	if (!HTTP_ParseRequestLine(client, requestLine))
	{
		return qfalse;
	}

	// Parse headers (skip \r\n after request line)
	headersStart += 2;
	if (!HTTP_ParseHeaders(client, headersStart))
	{
		return qfalse;
	}

	// Validate ET 2.60b client compatibility
	// Accept "ID_DOWNLOAD/2.0" user agent
	if (client->userAgent[0] != '\0')
	{
		if (strstr(client->userAgent, "ID_DOWNLOAD"))
		{
			Com_DPrintf("HTTP: ET 2.60b client detected\n");
		}
	}

	// Accept ET:// referer format
	if (client->referer[0] != '\0')
	{
		if (Q_stricmpn(client->referer, "et://", 5) == 0)
		{
			Com_DPrintf("HTTP: ET:// referer format detected\n");
		}
	}

	return qtrue;
}
