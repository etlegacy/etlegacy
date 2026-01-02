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
 */

/**
 * @file sv_http_ipresolve.c
 * @brief IP address resolution for the HTTP server
 */

#include "server.h"
#include "sv_http.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
typedef int SOCKET;
#define closesocket close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

/**
 * @brief Get the external IP address using STUN
 * @param[out] ipBuffer Buffer to store IP address string
 * @param[in] ipBufferSize Size of IP buffer
 * @return qtrue if the external IP was found, qfalse otherwise
 */
qboolean HTTP_GetExternalIP(char *ipBuffer, int ipBufferSize)
{
	// A list of prominent STUN servers.
	const char *stun_servers[] =
	{
		"stun.l.google.com",
		"stun1.l.google.com",
		"stun2.l.google.com",
		"stun3.l.google.com",
		"stun4.l.google.com",
		"stun.ekiga.net",
		"stun.ideasip.com",
		"stun.schlund.de",
		"stun.stunprotocol.org",
		"stun.voiparound.com",
		"stun.voipbuster.com",
		"stun.voipstunt.com",
		"stun.voxgratia.org",
		NULL
	};

	SOCKET sock = INVALID_SOCKET;
	int    i;

	if (!ipBuffer || ipBufferSize < 16)
	{
		return qfalse;
	}

	for (i = 0; stun_servers[i] != NULL; i++)
	{
		struct addrinfo         hints, *res = NULL;
		int                     err;
		unsigned char           request[20];
		unsigned char           response[2048];
		struct sockaddr_storage from;
		socklen_t               fromlen;
		struct timeval          tv;
		int                     len;

		Com_Memset(&hints, 0, sizeof(hints));
		hints.ai_family   = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;

		err = getaddrinfo(stun_servers[i], "3478", &hints, &res);
		if (err != 0)
		{
			Com_Printf("HTTP_GetExternalIP: getaddrinfo for %s failed: %s\n", stun_servers[i], gai_strerror(err));
			continue;
		}

		sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sock == INVALID_SOCKET)
		{
			freeaddrinfo(res);
			Com_Printf("HTTP_GetExternalIP: socket creation failed\n");
			continue;
		}

		// STUN request packet
		Com_Memset(request, 0, sizeof(request));
		*(unsigned short *)&request[0] = htons(0x0001);
		*(unsigned short *)&request[2] = htons(0x0000);
		*(unsigned int *)&request[4]   = htonl(0x2112A442);
		// Generate 12 random bytes for transaction ID (bytes 8-19)
		Com_RandomBytes(&request[8], 12);

		if (sendto(sock, (const char *)request, sizeof(request), 0, res->ai_addr, res->ai_addrlen) == SOCKET_ERROR)
		{
			Com_Printf("HTTP_GetExternalIP: sendto failed\n");
			closesocket(sock);
			freeaddrinfo(res);
			continue;
		}

		fromlen = sizeof(from);

		// Set a timeout for recvfrom
		tv.tv_sec  = 2; // 2 second timeout
		tv.tv_usec = 0;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

		len = recvfrom(sock, (char *)response, sizeof(response), 0, (struct sockaddr *)&from, &fromlen);

		closesocket(sock);
		freeaddrinfo(res);

		if (len > 0)
		{
			// Basic STUN response parsing
			if (len >= 20 && ntohs(*(unsigned short *)&response[0]) == 0x0101) // Binding Success Response
			{
				int pos = 20;
				while (pos + 4 <= len)
				{
					unsigned short attr_type = ntohs(*(unsigned short *)&response[pos]);
					unsigned short attr_len  = ntohs(*(unsigned short *)&response[pos + 2]);
					pos += 4;

					if (attr_type == 0x0001) // MAPPED-ADDRESS
					{
						if (attr_len >= 8 && response[pos + 1] == 0x01) // IPv4
						{
							struct in_addr addr;
							addr.s_addr = *(unsigned int *)&response[pos + 4];
							Q_strncpyz(ipBuffer, inet_ntoa(addr), ipBufferSize);
							return qtrue;
						}
					}
					pos += attr_len;
					// Align to 4-byte boundary
					if (pos % 4 != 0)
					{
						pos += 4 - (pos % 4);
					}
				}
			}
		}
		else
		{
			Com_Printf("HTTP_GetExternalIP: recvfrom failed or timed out for %s\n", stun_servers[i]);
		}
	}

	return qfalse;
}