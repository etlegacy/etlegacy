/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
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
 * @file net_ip.c
 */

#include "q_shared.h"
#include "qcommon.h"

#ifdef _WIN32
#   ifdef __MINGW32__
#       undef _WIN32_WINNT
#       define _WIN32_WINNT 0x0501
#       include <ws2spi.h>
#   endif
#   include <winsock2.h>
#   include <ws2tcpip.h>

typedef int socklen_t;
#   ifdef ADDRESS_FAMILY
#       define sa_family_t  ADDRESS_FAMILY
#   else
typedef unsigned short sa_family_t;
#   endif

#ifdef EAGAIN
#   undef EAGAIN
#endif
#define EAGAIN WSAEWOULDBLOCK

#ifdef EADDRNOTAVAIL
#   undef EADDRNOTAVAIL
#endif
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL

#ifdef EAFNOSUPPORT
#   undef EAFNOSUPPORT
#endif
#define EAFNOSUPPORT WSAEAFNOSUPPORT

#ifdef ECONNRESET
#   undef ECONNRESET
#endif
#define ECONNRESET WSAECONNRESET

#define socketError      WSAGetLastError()

static WSADATA  winsockdata;
static qboolean winsockInitialized = qfalse;

#else // *NIX & APPLE

#   if MAC_OS_X_VERSION_MIN_REQUIRED == 1020
// needed for socklen_t on OSX 10.2
#       define _BSD_SOCKLEN_T_
#   endif

#   include <sys/socket.h>
#   include <netinet/tcp.h>
#   include <errno.h>
#   include <netdb.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <net/if.h>
#   include <sys/ioctl.h>
#   include <sys/types.h>
#   include <sys/time.h>
#   include <unistd.h>
#   if !defined(__sun) && !defined(__sgi)
#       include <ifaddrs.h>
#   endif

#   ifdef __sun
#       include <sys/filio.h>
#   endif

typedef int SOCKET;
#   define INVALID_SOCKET       -1
#   define SOCKET_ERROR         -1
#   define closesocket          close
#   define ioctlsocket          ioctl
#   define socketError          errno

#endif

static qboolean usingSocks        = qfalse;
static qboolean networkingEnabled = qfalse;

static cvar_t *net_enabled;

static cvar_t *net_socksEnabled;
static cvar_t *net_socksServer;
static cvar_t *net_socksPort;
static cvar_t *net_socksUsername;
static cvar_t *net_socksPassword;

static cvar_t *net_ip;
static cvar_t *net_port;

#ifdef FEATURE_IPV6
static cvar_t *net_ip6;
static cvar_t *net_port6;
static cvar_t *net_mcast6addr;
static cvar_t *net_mcast6iface;
#endif

static cvar_t *net_dropsim; // 0.0 to 1.0, simulated packet drops

static struct sockaddr socksRelayAddr;

static SOCKET ip_socket    = INVALID_SOCKET;
static SOCKET socks_socket = INVALID_SOCKET;

#ifdef FEATURE_IPV6
static SOCKET ip6_socket        = INVALID_SOCKET;
static SOCKET multicast6_socket = INVALID_SOCKET;

// Keep track of currently joined multicast group.
static struct ipv6_mreq curgroup;
// And the currently bound address.
static struct sockaddr_in6 boundto;

// use an admin local address per default so that network admins can decide on how to handle quake3 traffic.
#define NET_MULTICAST_IP6 "ff04::696f:7175:616b:6533"
#endif

#ifndef IF_NAMESIZE
  #define IF_NAMESIZE 16
#endif

#define MAX_IPS     32

typedef struct
{
	char ifname[IF_NAMESIZE];

	netadrtype_t type;
	sa_family_t family;
	struct sockaddr_storage addr;
	struct sockaddr_storage netmask;
} nip_localaddr_t;

static nip_localaddr_t localIP[MAX_IPS];
static int             numIP;

//=============================================================================

/**
 * @brief NET_ErrorString
 * @return
 */
char *NET_ErrorString(void)
{
#ifdef _WIN32
	//FIXME: replace with FormatMessage?
	switch (socketError)
	{
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
#else
	return strerror(errno);
#endif
}

/**
 * @brief NetadrToSockadr
 * @param[in] a
 * @param[out] s
 */
static void NetadrToSockadr(netadr_t *a, struct sockaddr *s)
{
	switch (a->type)
	{
	case NA_BROADCAST:
		((struct sockaddr_in *)s)->sin_family      = AF_INET;
		((struct sockaddr_in *)s)->sin_port        = a->port;
		((struct sockaddr_in *)s)->sin_addr.s_addr = INADDR_BROADCAST;
		break;
	case NA_IP:
		((struct sockaddr_in *)s)->sin_family      = AF_INET;
		((struct sockaddr_in *)s)->sin_addr.s_addr = *(int *)&a->ip;
		((struct sockaddr_in *)s)->sin_port        = a->port;
		break;
#ifdef FEATURE_IPV6
	case NA_IP6:
		((struct sockaddr_in6 *)s)->sin6_family   = AF_INET6;
		((struct sockaddr_in6 *)s)->sin6_addr     = *((struct in6_addr *) &a->ip6);
		((struct sockaddr_in6 *)s)->sin6_port     = a->port;
		((struct sockaddr_in6 *)s)->sin6_scope_id = a->scope_id;
		break;
	case NA_MULTICAST6:
		((struct sockaddr_in6 *)s)->sin6_family = AF_INET6;
		((struct sockaddr_in6 *)s)->sin6_addr   = curgroup.ipv6mr_multiaddr;
		((struct sockaddr_in6 *)s)->sin6_port   = a->port;
		break;
#endif
	default:
		Com_Printf("NetadrToSockadr: bad address type\n");
		break;
	}
}

/**
 * @brief SockadrToNetadr
 * @param[in] s
 * @param[out] a
 */
static void SockadrToNetadr(struct sockaddr *s, netadr_t *a)
{
	if (s->sa_family == AF_INET)
	{
		a->type        = NA_IP;
		*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		a->port        = ((struct sockaddr_in *)s)->sin_port;
	}
#ifdef FEATURE_IPV6
	else if (s->sa_family == AF_INET6)
	{
		a->type = NA_IP6;
		Com_Memcpy(a->ip6, &((struct sockaddr_in6 *)s)->sin6_addr, sizeof(a->ip6));
		a->port     = ((struct sockaddr_in6 *)s)->sin6_port;
		a->scope_id = ((struct sockaddr_in6 *)s)->sin6_scope_id;
	}
#endif
}

#ifdef FEATURE_IPV6
static struct addrinfo *SearchAddrInfo(struct addrinfo *hints, sa_family_t family)
{
	while (hints)
	{
		if (hints->ai_family == family)
		{
			return hints;
		}

		hints = hints->ai_next;
	}

	return NULL;
}
#endif

/**
 * @brief Sys_StringToSockaddr
 * @param[in] s
 * @param[out] sadr
 * @param[in] sadr_len
 * @param[in] family
 * @return
 */
static qboolean Sys_StringToSockaddr(const char *s, struct sockaddr *sadr, int sadr_len, sa_family_t family)
{
	struct addrinfo hints;              // provides hints about the type of socket the caller supports
	struct addrinfo *res = NULL;        // contains response information about the host
#ifdef FEATURE_IPV6
	struct addrinfo *search = NULL;
	struct addrinfo *hintsp;
	int             retval;

	Com_Memset(sadr, '\0', sizeof(*sadr));
	Com_Memset(&hints, '\0', sizeof(hints));

	hintsp              = &hints;
	hintsp->ai_family   = family;
	hintsp->ai_socktype = SOCK_DGRAM;

	retval = getaddrinfo(s, NULL, hintsp, &res);

	// Network needs to be init before this, and it should have been.
	if (!networkingEnabled)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: Sys_StringToSockaddr: Networking is not initialized\n");
		etl_assert(qfalse);
		return qfalse;
	}

	if (!retval)
	{
		if (family == AF_UNSPEC)
		{
			// Decide here and now which protocol family to use
			if (net_enabled->integer & NET_PRIOV6)
			{
				if (net_enabled->integer & NET_ENABLEV6)
				{
					search = SearchAddrInfo(res, AF_INET6);
				}

				if (!search && (net_enabled->integer & NET_ENABLEV4))
				{
					search = SearchAddrInfo(res, AF_INET);
				}
			}
			else
			{
				if (net_enabled->integer & NET_ENABLEV4)
				{
					search = SearchAddrInfo(res, AF_INET);
				}

				if (!search && (net_enabled->integer & NET_ENABLEV6))
				{
					search = SearchAddrInfo(res, AF_INET6);
				}
			}
		}
		else
		{
			search = SearchAddrInfo(res, family);
		}

		if (search)
		{
			if (res->ai_addrlen > sadr_len)
			{
				res->ai_addrlen = sadr_len;
			}

			Com_Memcpy(sadr, res->ai_addr, res->ai_addrlen);
			freeaddrinfo(res);

			return qtrue;
		}
		else
		{
			Com_Printf("Sys_StringToSockaddr: Error resolving %s: No address of required type found\n", s);
		}
	}
	else
	{
		Com_Printf("Sys_StringToSockaddr: Error resolving %s: %s\n", s, gai_strerror(retval));
	}

	if (res)
	{
		freeaddrinfo(res);
	}

	return qfalse;
#else // IPV4

	Com_Memset(sadr, 0, sizeof(*sadr));
	((struct sockaddr_in *)sadr)->sin_family = AF_INET;
	((struct sockaddr_in *)sadr)->sin_port   = 0;

	Com_Memset(&hints, '\0', sizeof(hints));
	hints.ai_family   = family;
	hints.ai_socktype = SOCK_DGRAM;

	if (s[0] >= '0' && s[0] <= '9')
	{
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr(s);
	}
	else
	{
		if (getaddrinfo(s, NULL, &hints, &res))
		{
			return qfalse;
		}
		Com_Memcpy(sadr, res->ai_addr, res->ai_addrlen);
		freeaddrinfo(res);
	}

	return qtrue;
#endif
}

/**
 * @brief Copies ANSI host name into dest of address familiy IPv4 or IPv6
 * @param[out] dest
 * @param[in] destlen
 * @param[in] input
 */
static void Sys_SockaddrToString(char *dest, size_t destlen, struct sockaddr *input)
{
	socklen_t inputlen;

	if (input->sa_family == AF_INET6) // FEATURE_IPV6
	{
		inputlen = sizeof(struct sockaddr_in6);
	}
	else if (input->sa_family == AF_INET)
	{
		inputlen = sizeof(struct sockaddr_in);
	}
	else
	{
		Com_Error(ERR_FATAL, "Sys_SockaddrToString: unsupported address family");
	}

	if (getnameinfo(input, inputlen, dest, destlen, NULL, 0, NI_NUMERICHOST) && destlen > 0)
	{
		*dest = '\0';
	}
}

/**
 * @brief Sys_StringToAdr
 * @param[in] s
 * @param[in] a
 * @param[in] family
 * @return
 */
qboolean Sys_StringToAdr(const char *s, netadr_t *a, netadrtype_t family)
{
	struct sockaddr_storage sadr;
	sa_family_t             fam;

	switch (family)
	{
	case NA_IP:
		fam = AF_INET;
		break;
#ifdef FEATURE_IPV6
	case NA_IP6:
		fam = AF_INET6;
		break;
#endif
	default:
		fam = AF_UNSPEC;
		break;
	}
	if (!Sys_StringToSockaddr(s, (struct sockaddr *) &sadr, sizeof(sadr), fam))
	{
		return qfalse;
	}

	SockadrToNetadr((struct sockaddr *) &sadr, a);
	return qtrue;
}

/**
 * @brief Compare without port, and up to the bit number given in netmask.
 * @param[in] a
 * @param[in] b
 * @param[in] netmask
 * @return
 */
qboolean NET_CompareBaseAdrMask(netadr_t a, netadr_t b, int netmask)
{
	qboolean differed;
	byte     cmpmask, *addra, *addrb;
	int      curbyte;

	if (a.type != b.type)
	{
		return qfalse;
	}

	switch (a.type)
	{
	case NA_LOOPBACK:
		return qtrue;
	case NA_IP:
	{
		addra = (byte *) &a.ip;
		addrb = (byte *) &b.ip;

		if (netmask < 0 || netmask > 32)
		{
			netmask = 32;
		}
	}
	break;
#ifdef FEATURE_IPV6
	case NA_IP6:
	{
		addra = (byte *) &a.ip6;
		addrb = (byte *) &b.ip6;

		if (netmask < 0 || netmask > 128)
		{
			netmask = 128;
		}
	}
	break;
#endif
	default:
		Com_Printf("NET_CompareBaseAdrMask: bad address type a: %i (b: %i, netmask: %i)\n", a.type, b.type, netmask);
		return qfalse;
	}

	differed = qfalse;
	curbyte  = 0;

	while (netmask > 7)
	{
		if (addra[curbyte] != addrb[curbyte])
		{
			differed = qtrue;
			break;
		}

		curbyte++;
		netmask -= 8;
	}

	if (differed)
	{
		return qfalse;
	}

	if (netmask)
	{
		cmpmask   = (1 << netmask) - 1;
		cmpmask <<= 8 - netmask;

		if ((addra[curbyte] & cmpmask) == (addrb[curbyte] & cmpmask))
		{
			return qtrue;
		}
	}
	else
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief Compares ip addresses without the port.
 * @param[in] a
 * @param[in] b
 * @return
 */
qboolean NET_CompareBaseAdr(netadr_t a, netadr_t b)
{
	return NET_CompareBaseAdrMask(a, b, -1);
}

/**
 * @brief NET_AdrToStringNoPort
 * @param[in] a
 * @return
 */
const char *NET_AdrToStringNoPort(netadr_t a)
{
	static char s[NET_ADDRSTRMAXLEN];

	switch (a.type)
	{
	case NA_LOOPBACK:
		Com_sprintf(s, sizeof(s), "loopback");
		break;
	case NA_BOT:
		Com_sprintf(s, sizeof(s), "bot");
		break;
	case NA_IP:
		Com_sprintf(s, sizeof(s), "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);
		break;
#ifdef FEATURE_IPV6
	case NA_IP6:
	{
		struct sockaddr_storage sadr;

		Com_Memset(&sadr, 0, sizeof(sadr));
		NetadrToSockadr(&a, (struct sockaddr *) &sadr);
		Sys_SockaddrToString(s, sizeof(s), (struct sockaddr *) &sadr);
		break;
	}
	break;
#endif
	case NA_BAD: // Invalid, unknown or non-applicable address type
		//Com_Printf("NET_AdrToString: Address type: 0.0.0.0 or ::\n");
		Com_sprintf(s, sizeof(s), "invalid");
		break;
	default:
		Com_Printf("NET_AdrToStringNoPort: Unknown address type: %i\n", a.type);
		Com_sprintf(s, sizeof(s), "unknown");
		break;
	}

	return s;
}

/**
 * @brief Returns address & port
 * @param[in] a
 * @return
 */
const char *NET_AdrToString(netadr_t a)
{
	static char s[NET_ADDRSTRMAXLEN_EXT];

	switch (a.type)
	{
	case NA_LOOPBACK:
		Com_sprintf(s, sizeof(s), "loopback");
		break;
	case NA_BOT:
		Com_sprintf(s, sizeof(s), "bot");
		break;
	case NA_IP:
		Com_sprintf(s, sizeof(s), "%i.%i.%i.%i:%hu", a.ip[0], a.ip[1], a.ip[2], a.ip[3], BigShort(a.port));
		break;
#ifdef FEATURE_IPV6
	case NA_IP6:
		Com_sprintf(s, sizeof(s), "[%s]:%hu", NET_AdrToStringNoPort(a), ntohs(a.port));
		break;
#endif
	case NA_BAD: // Invalid, unknown or non-applicable address type
		//Com_Printf("NET_AdrToString: Address type: 0.0.0.0 or ::\n");
		Com_sprintf(s, sizeof(s), "invalid");
		break;
	default:
		Com_Printf("NET_AdrToString: Unknown address type: %i\n", a.type);
		Com_sprintf(s, sizeof(s), "unknown");
		break;
	}

	return s;
}

/**
 * @brief NET_CompareAdr
 * @param[in] a
 * @param[in] b
 * @return
 */
qboolean NET_CompareAdr(netadr_t a, netadr_t b)
{
	if (!NET_CompareBaseAdr(a, b))
	{
		return qfalse;
	}

	if (a.type == NA_IP
#ifdef FEATURE_IPV6
	    || a.type == NA_IP6
#endif
	    )
	{
		if (a.port == b.port)
		{
			return qtrue;
		}
	}
	else
	{
		return qtrue;
	}

	return qfalse;
}

/**
 * @brief NET_IsLocalAddress
 * @param[in] adr
 * @return
 */
qboolean NET_IsLocalAddress(netadr_t adr)
{
	return adr.type == NA_LOOPBACK;
}

/**
 * @brief NET_IsLocalAddressString
 * @param[in] address
 * @return
 */
qboolean NET_IsLocalAddressString(const char *address)
{
	// see NET_AdrToString & make fail safe (see UI)
	return !Q_stricmp(address, "loopback") || !Q_stricmp(address, "localhost");
}

//=============================================================================

/**
 * @brief Receive one packet
 * @param[in,out] net_from
 * @param[in,out] net_message
 * @param[in] fdr
 * @return
 */
qboolean NET_GetPacket(netadr_t *net_from, msg_t *net_message, fd_set *fdr)
{
	int                     ret;
	struct sockaddr_storage from;
	socklen_t               fromlen;
	int                     err;

	if (ip_socket != INVALID_SOCKET && FD_ISSET(ip_socket, fdr))
	{
		fromlen = sizeof(from);
		ret     = recvfrom(ip_socket, (void *)net_message->data, net_message->maxsize, 0, (struct sockaddr *) &from, &fromlen);

		if (ret == SOCKET_ERROR)
		{
			err = socketError;

			if (err != EAGAIN && err != ECONNRESET)
			{
				Com_Printf("NET_GetPacket: %s\n", NET_ErrorString());
			}
		}
		else
		{
			Com_Memset(((struct sockaddr_in *)&from)->sin_zero, 0, 8);

			if (usingSocks && memcmp(&from, &socksRelayAddr, fromlen) == 0)
			{
				if (ret < 10 || net_message->data[0] != 0 || net_message->data[1] != 0 || net_message->data[2] != 0 || net_message->data[3] != 1)
				{
					return qfalse;
				}
				net_from->type         = NA_IP;
				net_from->ip[0]        = net_message->data[4];
				net_from->ip[1]        = net_message->data[5];
				net_from->ip[2]        = net_message->data[6];
				net_from->ip[3]        = net_message->data[7];
				net_from->port         = *(short *)&net_message->data[8];
				net_message->readcount = 10;
			}
			else
			{
				SockadrToNetadr((struct sockaddr *) &from, net_from);
				net_message->readcount = 0;
			}

			if (ret >= net_message->maxsize)
			{
				Com_Printf("Oversize packet from %s\n", NET_AdrToString(*net_from));
				return qfalse;
			}

			net_message->cursize = ret;
			return qtrue;
		}
	}

#ifdef FEATURE_IPV6
	if (ip6_socket != INVALID_SOCKET && FD_ISSET(ip6_socket, fdr))
	{
		fromlen = sizeof(from);
		ret     = recvfrom(ip6_socket, (void *)net_message->data, net_message->maxsize, 0, (struct sockaddr *) &from, &fromlen);

		if (ret == SOCKET_ERROR)
		{
			err = socketError;

			if (err != EAGAIN && err != ECONNRESET)
			{
				Com_Printf("NET_GetPacket: %s\n", NET_ErrorString());
			}
		}
		else
		{
			SockadrToNetadr((struct sockaddr *) &from, net_from);
			net_message->readcount = 0;

			if (ret >= net_message->maxsize)
			{
				Com_Printf("Oversize packet from %s\n", NET_AdrToString(*net_from));
				return qfalse;
			}

			net_message->cursize = ret;
			return qtrue;
		}
	}

	if (multicast6_socket != INVALID_SOCKET && multicast6_socket != ip6_socket && FD_ISSET(multicast6_socket, fdr))
	{
		fromlen = sizeof(from);
		ret     = recvfrom(multicast6_socket, (void *)net_message->data, net_message->maxsize, 0, (struct sockaddr *) &from, &fromlen);

		if (ret == SOCKET_ERROR)
		{
			err = socketError;

			if (err != EAGAIN && err != ECONNRESET)
			{
				Com_Printf("NET_GetPacket: %s\n", NET_ErrorString());
			}
		}
		else
		{
			SockadrToNetadr((struct sockaddr *) &from, net_from);
			net_message->readcount = 0;

			if (ret >= net_message->maxsize)
			{
				Com_Printf("Oversize packet from %s\n", NET_AdrToString(*net_from));
				return qfalse;
			}

			net_message->cursize = ret;
			return qtrue;
		}
	}
#endif

	return qfalse;
}

//=============================================================================

static char socksBuf[4096];

/**
 * @brief Sys_SendPacket
 * @param[in] length
 * @param[in] data
 * @param[in] to
 */
void Sys_SendPacket(int length, const void *data, netadr_t to)
{
	int                     ret = SOCKET_ERROR;
	struct sockaddr_storage addr;

	if (to.type != NA_BROADCAST && to.type != NA_IP
#ifdef FEATURE_IPV6
	    && to.type != NA_IP6 && to.type != NA_MULTICAST6
#endif
	    )
	{
		Com_Error(ERR_FATAL, "Sys_SendPacket: bad address type");
	}

	if ((ip_socket == INVALID_SOCKET && to.type == NA_IP) ||
	    (ip_socket == INVALID_SOCKET && to.type == NA_BROADCAST))
	{
		return;
	}

#ifdef FEATURE_IPV6
	if ((ip6_socket == INVALID_SOCKET && to.type == NA_IP6) ||
	    (ip6_socket == INVALID_SOCKET && to.type == NA_MULTICAST6))
	{
		return;
	}

	if (to.type == NA_MULTICAST6 && (net_enabled->integer & NET_DISABLEMCAST))
	{
		return;
	}
#endif

	Com_Memset(&addr, 0, sizeof(addr));
	NetadrToSockadr(&to, (struct sockaddr *) &addr);

	if (usingSocks && to.type == NA_IP)
	{
		socksBuf[0]            = 0; // reserved
		socksBuf[1]            = 0;
		socksBuf[2]            = 0; // fragment (not fragmented)
		socksBuf[3]            = 1; // address type: IPV4
		*(int *)&socksBuf[4]   = ((struct sockaddr_in *)&addr)->sin_addr.s_addr;
		*(short *)&socksBuf[8] = ((struct sockaddr_in *)&addr)->sin_port;
		Com_Memcpy(&socksBuf[10], data, length);
		ret = sendto(ip_socket, socksBuf, length + 10, 0, &socksRelayAddr, sizeof(socksRelayAddr));
	}
	else
	{
		if (addr.ss_family == AF_INET)
		{
			ret = sendto(ip_socket, data, length, 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
		}
#ifdef FEATURE_IPV6
		else if (addr.ss_family == AF_INET6)
		{
			ret = sendto(ip6_socket, data, length, 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in6));
		}
#endif
	}
	if (ret == SOCKET_ERROR)
	{
		int err = socketError;

		// wouldblock is silent
		if (err == EAGAIN)
		{
			return;
		}

		// some PPP links do not allow broadcasts and return an error
		if ((err == EADDRNOTAVAIL) && ((to.type == NA_BROADCAST)))
		{
			return;
		}

		Com_Printf("Sys_SendPacket: %s\n", NET_ErrorString());
	}
}

//=============================================================================

/**
 * @brief	LAN clients will have their rate var ignored
 *
 *          IPv4: A LAN client is loopback adapter (localhost/127.0.0.1),
 *                a computer of the network this computer is member of
 *                or private network from RFC1918:
 *          10.0.0.0        -   10.255.255.255  (10/8 prefix)
 *	        172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
 *			192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
 *
 *          IPv6:               FIXME: docu
 *
 *          This function expects adresses of type NA_IP_4 & 6 ONLY!
 *          other types (also NA_BOT) will return qfalse
 *
 * @param[in] adr
 *
 * @todo docu IPv6
 */
qboolean Sys_IsLANAddress(netadr_t adr)
{
	int      index, run, addrsize = 0;
	qboolean differed;
	byte     *compareadr = NULL, *comparemask = NULL, *compareip = NULL;

	switch (adr.type)
	{
	case NA_LOOPBACK:
		return qtrue;
	case NA_IP:
		// RFC1918:
		if (adr.ip[0] == 10)
		{
			return qtrue;
		}
		if (adr.ip[0] == 172 && (adr.ip[1] & 0xf0) == 16)
		{
			return qtrue;
		}
		if (adr.ip[0] == 192 && adr.ip[1] == 168)
		{
			return qtrue;
		}

		if (adr.ip[0] == 127)
		{
			return qtrue;
		}
		break;
	case NA_IP6:
#ifdef FEATURE_IPV6
		if (adr.ip6[0] == 0xfe && (adr.ip6[1] & 0xc0) == 0x80)
		{
			return qtrue;
		}
		if ((adr.ip6[0] & 0xfe) == 0xfc)
		{
			return qtrue;
		}
		break;
#else
		return qfalse; // NA_IP6 disabled
#endif
	default: // drop broadcast & other unwanted types
		return qfalse;
	}

	// Now compare against the networks this computer is member of.
	for (index = 0; index < numIP; index++)
	{
		if (localIP[index].type == adr.type)
		{
			if (adr.type == NA_IP)
			{
				compareip   = (byte *) &((struct sockaddr_in *) &localIP[index].addr)->sin_addr.s_addr;
				comparemask = (byte *) &((struct sockaddr_in *) &localIP[index].netmask)->sin_addr.s_addr;
				compareadr  = adr.ip;

				addrsize = sizeof(adr.ip);
			}
			else
			{
#ifdef FEATURE_IPV6
				// TODO? should we check the scope_id here?

				compareip   = (byte *) &((struct sockaddr_in6 *) &localIP[index].addr)->sin6_addr;
				comparemask = (byte *) &((struct sockaddr_in6 *) &localIP[index].netmask)->sin6_addr;
				compareadr  = adr.ip6;

				addrsize = sizeof(adr.ip6);
#endif
			}

			differed = qfalse;
			for (run = 0; run < addrsize; run++)
			{
				if ((compareip[run] & comparemask[run]) != (compareadr[run] & comparemask[run]))
				{
					differed = qtrue;
					break;
				}
			}

			if (!differed)
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

/**
 * @brief Sys_ShowIP
 */
void Sys_ShowIP(void)
{
	int  i;
	char addrbuf[NET_ADDRSTRMAXLEN];

	for (i = 0; i < numIP; i++)
	{
		Sys_SockaddrToString(addrbuf, sizeof(addrbuf), (struct sockaddr *) &localIP[i].addr);

		if (localIP[i].type == NA_IP)
		{
			Com_Printf("IP: %s\n", addrbuf);
		}
#ifdef FEATURE_IPV6
		else if (localIP[i].type == NA_IP6)
		{
			Com_Printf("IP6: %s\n", addrbuf);
		}
#endif
	}
}

//=============================================================================

/**
 * @brief NET_IPSocket
 * @param[in] net_interface
 * @param[in] port
 * @param[out] err
 * @return
 */
int NET_IPSocket(const char *net_interface, int port, int *err)
{
	SOCKET             newsocket;
	struct sockaddr_in address;
	u_long             _true = 1;
	int                i     = 1;

	struct timeval timeout;
	timeout.tv_sec  = 1;
	timeout.tv_usec = 0;

	*err = 0;

	if (net_interface)
	{
		Com_Printf("Opening IP socket: %s:%i\n", net_interface, port);
	}
	else
	{
		Com_Printf("Opening IP socket: 0.0.0.0:%i\n", port);
	}

	if ((newsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		*err = socketError;
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_IPSocket - socket: %s\n", NET_ErrorString());
		return newsocket;
	}
	// make it non-blocking
	if (ioctlsocket(newsocket, FIONBIO, &_true) == SOCKET_ERROR)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_IPSocket - ioctl FIONBIO: %s\n", NET_ErrorString());
		*err = socketError;
		closesocket(newsocket);
		return INVALID_SOCKET;
	}

	// make it broadcast capable
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == SOCKET_ERROR)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_IPSocket - setsockopt SO_BROADCAST: %s\n", NET_ErrorString());
	}

	// set socket timeout
#ifdef _WIN32
	timeout.tv_sec *= 1000; // win32 uses msec
#endif
	if (setsockopt(newsocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_IPSocket - can't set RCVTIMEO: %s\n", NET_ErrorString());
	}

	if (!net_interface || !net_interface[0])
	{
		address.sin_family      = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		if (!Sys_StringToSockaddr(net_interface, (struct sockaddr *)&address, sizeof(address), AF_INET))
		{
			closesocket(newsocket);
			return INVALID_SOCKET;
		}
	}

	if (port == PORT_ANY)
	{
		address.sin_port = 0;
	}
	else
	{
		address.sin_port = htons((short)port);
	}

	if (bind(newsocket, (void *)&address, sizeof(address)) == SOCKET_ERROR)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_IPSocket - bind: %s\n", NET_ErrorString());
		*err = socketError;
		closesocket(newsocket);
		return INVALID_SOCKET;
	}

	return newsocket;
}

#ifdef FEATURE_IPV6

/**
 * @brief NET_IP6Socket
 * @param[in] net_interface
 * @param[in] port
 * @param[in] bindto
 * @param[out] err
 * @return
 */
int NET_IP6Socket(const char *net_interface, int port, struct sockaddr_in6 *bindto, int *err)
{
	SOCKET              newsocket;
	struct sockaddr_in6 address;
	u_long              _true = 1;

	*err = 0;

	if (net_interface)
	{
		// Print the name in brackets if there is a colon:
		if (Q_CountChar(net_interface, ':'))
		{
			Com_Printf("Opening IP6 socket: [%s]:%i\n", net_interface, port);
		}
		else
		{
			Com_Printf("Opening IP6 socket: %s:%i\n", net_interface, port);
		}
	}
	else
	{
		Com_Printf("Opening IP6 socket: [::]:%i\n", port);
	}

	if ((newsocket = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		*err = socketError;
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_IP6Socket: socket: %s\n", NET_ErrorString());
		return newsocket;
	}

	// make it non-blocking
	if (ioctlsocket(newsocket, FIONBIO, &_true) == SOCKET_ERROR)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_IP6Socket: ioctl FIONBIO: %s\n", NET_ErrorString());
		*err = socketError;
		closesocket(newsocket);
		return INVALID_SOCKET;
	}

#ifdef IPV6_V6ONLY
	{
		int i = 1;

		// ipv4 addresses should not be allowed to connect via this socket.
		if (setsockopt(newsocket, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &i, sizeof(i)) == SOCKET_ERROR)
		{
			// win32 systems don't seem to support this anyways.
			Com_Printf(S_COLOR_YELLOW "WARNING: NET_IP6Socket: setsockopt IPV6_V6ONLY: %s\n", NET_ErrorString());
		}
	}
#endif

	if (!net_interface || !net_interface[0])
	{
		address.sin6_family = AF_INET6;
		address.sin6_addr   = in6addr_any;
	}
	else
	{
		if (!Sys_StringToSockaddr(net_interface, (struct sockaddr *)&address, sizeof(address), AF_INET6))
		{
			closesocket(newsocket);
			return INVALID_SOCKET;
		}
	}

	if (port == PORT_ANY)
	{
		address.sin6_port = 0;
	}
	else
	{
		address.sin6_port = htons((short)port);
	}

	if (bind(newsocket, (void *)&address, sizeof(address)) == SOCKET_ERROR)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_IP6Socket: bind: %s\n", NET_ErrorString());
		*err = socketError;
		closesocket(newsocket);
		return INVALID_SOCKET;
	}

	if (bindto)
	{
		*bindto = address;
	}

	return newsocket;
}

/**
 * @brief Set the current multicast group
 */
void NET_SetMulticast6(void)
{
	struct sockaddr_in6 addr;

	if (!*net_mcast6addr->string || !Sys_StringToSockaddr(net_mcast6addr->string, (struct sockaddr *) &addr, sizeof(addr), AF_INET6))
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_JoinMulticast6: Incorrect multicast address given, "
		           "please set cvar %s to a sane value.\n", net_mcast6addr->name);

		Cvar_SetValue(net_enabled->name, net_enabled->integer | NET_DISABLEMCAST);

		return;
	}

	Com_Memcpy(&curgroup.ipv6mr_multiaddr, &addr.sin6_addr, sizeof(curgroup.ipv6mr_multiaddr));

	if (*net_mcast6iface->string)
	{
#ifdef _WIN32
		curgroup.ipv6mr_interface = net_mcast6iface->integer;
#else
		curgroup.ipv6mr_interface = if_nametoindex(net_mcast6iface->string);
#endif
	}
	else
	{
		curgroup.ipv6mr_interface = 0;
	}
}

/**
 * @brief Join an ipv6 multicast group
 */
void NET_JoinMulticast6(void)
{
	int err;

	if (ip6_socket == INVALID_SOCKET || multicast6_socket != INVALID_SOCKET || (net_enabled->integer & NET_DISABLEMCAST))
	{
		return;
	}

	if (IN6_IS_ADDR_MULTICAST(&boundto.sin6_addr) || IN6_IS_ADDR_UNSPECIFIED(&boundto.sin6_addr))
	{
		// The way the socket was bound does not prohibit receiving multi-cast packets. So we don't need to open a new one.
		multicast6_socket = ip6_socket;
	}
	else
	{
		if ((multicast6_socket = NET_IP6Socket(net_mcast6addr->string, ntohs(boundto.sin6_port), NULL, &err)) == INVALID_SOCKET)
		{
			// If the OS does not support binding to multicast addresses, like WinXP, at least try with the normal file descriptor.
			multicast6_socket = ip6_socket;
		}
	}

	if (curgroup.ipv6mr_interface)
	{
		if (setsockopt(multicast6_socket, IPPROTO_IPV6, IPV6_MULTICAST_IF,
		               (char *) &curgroup.ipv6mr_interface, sizeof(curgroup.ipv6mr_interface)) < 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: NET_JoinMulticast6: Couldn't set scope on multicast socket: %s\n", NET_ErrorString());

			if (multicast6_socket != ip6_socket)
			{
				closesocket(multicast6_socket);
				multicast6_socket = INVALID_SOCKET;
				return;
			}
		}
	}

	if (setsockopt(multicast6_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char *) &curgroup, sizeof(curgroup)))
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_JoinMulticast6: Couldn't join multicast group: %s\n", NET_ErrorString());

		if (multicast6_socket != ip6_socket)
		{
			closesocket(multicast6_socket);
			multicast6_socket = INVALID_SOCKET;
			return;
		}
	}
}

/**
 * @brief NET_LeaveMulticast6
 */
void NET_LeaveMulticast6(void)
{
	if (multicast6_socket != INVALID_SOCKET)
	{
		if (multicast6_socket != ip6_socket)
		{
			closesocket(multicast6_socket);
		}
		else
		{
			setsockopt(multicast6_socket, IPPROTO_IPV6, IPV6_LEAVE_GROUP, (char *) &curgroup, sizeof(curgroup));
		}

		multicast6_socket = INVALID_SOCKET;
	}
}
#endif // FEATURE_IPV6

/**
 * @brief NET_OpenSocks
 * @param[in] port
 */
void NET_OpenSocks(int port)
{
	struct addrinfo hints;              // provides hints about the type of socket the caller supports
	struct addrinfo *res = NULL;        // contains response information about the host
	int             len;
	qboolean        rfc1929;
	unsigned char   buf[64];
	int             i = 1;

	Com_Memset(&hints, '\0', sizeof(hints));

	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	usingSocks = qfalse;

	Com_Printf("Opening connection to SOCKS server.\n");

	if ((socks_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: socket: %s\n", NET_ErrorString());
		return;
	}

	// Disable Nagle for this TCP socket
	if (setsockopt(socks_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&i, sizeof(i)) == SOCKET_ERROR)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: setsockopt: %s\n", NET_ErrorString());
		return;
	}

	if (getaddrinfo(net_socksServer->string, net_socksPort->string, &hints, &res) != 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: getaddrinfo: %s\n", NET_ErrorString());
		return;
	}

	if (res->ai_addr->sa_family != AF_INET)
	{
		freeaddrinfo(res);
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: getaddrinfo: address type was not AF_INET\n");
		return;
	}

	if (connect(socks_socket, res->ai_addr, res->ai_addrlen) != 0)
	{
		freeaddrinfo(res);
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: connect: %s\n", NET_ErrorString());
		return;
	}

	freeaddrinfo(res);

	// send socks authentication handshake
	if (*net_socksUsername->string || *net_socksPassword->string)
	{
		rfc1929 = qtrue;
	}
	else
	{
		rfc1929 = qfalse;
	}

	buf[0] = 5;     // SOCKS version
	// method count
	if (rfc1929)
	{
		buf[1] = 2;
		len    = 4;
	}
	else
	{
		buf[1] = 1;
		len    = 3;
	}
	buf[2] = 0;     // method #1 - method id #00: no authentication
	if (rfc1929)
	{
		buf[2] = 2;     // method #2 - method id #02: username/password
	}
	if (send(socks_socket, (void *)buf, len, 0) == SOCKET_ERROR)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: send: %s\n", NET_ErrorString());
		return;
	}

	// get the response
	len = recv(socks_socket, (void *)buf, 64, 0);
	if (len == SOCKET_ERROR)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: recv: %s\n", NET_ErrorString());
		return;
	}
	if (len != 2 || buf[0] != 5)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: bad response\n");
		return;
	}
	switch (buf[1])
	{
	case 0: // no authentication
		break;
	case 2: // username/password authentication
		break;
	default:
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: request denied\n");
		return;
	}

	// do username/password authentication if needed
	if (buf[1] == 2)
	{
		int ulen;
		int plen;

		// build the request
		ulen = strlen(net_socksUsername->string);
		plen = strlen(net_socksPassword->string);

		buf[0] = 1;     // username/password authentication version
		buf[1] = ulen;
		if (ulen)
		{
			Com_Memcpy(&buf[2], net_socksUsername->string, ulen);
		}
		buf[2 + ulen] = plen;
		if (plen)
		{
			Com_Memcpy(&buf[3 + ulen], net_socksPassword->string, plen);
		}

		// send it
		if (send(socks_socket, (void *)buf, 3 + ulen + plen, 0) == SOCKET_ERROR)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: send: %s\n", NET_ErrorString());
			return;
		}

		// get the response
		len = recv(socks_socket, (void *)buf, 64, 0);
		if (len == SOCKET_ERROR)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: recv: %s\n", NET_ErrorString());
			return;
		}
		if (len != 2 || buf[0] != 1)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: bad response\n");
			return;
		}
		if (buf[1] != 0)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: authentication failed\n");
			return;
		}
	}

	// send the UDP associate request
	buf[0]            = 5; // SOCKS version
	buf[1]            = 3; // command: UDP associate
	buf[2]            = 0; // reserved
	buf[3]            = 1; // address type: IPV4
	*(int *)&buf[4]   = INADDR_ANY;
	*(short *)&buf[8] = htons((short)port);         // port
	if (send(socks_socket, (void *)buf, 10, 0) == SOCKET_ERROR)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: send: %s\n", NET_ErrorString());
		return;
	}

	// get the response
	len = recv(socks_socket, (void *)buf, 64, 0);
	if (len == SOCKET_ERROR)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: recv: %s\n", NET_ErrorString());
		return;
	}
	if (len < 2 || buf[0] != 5)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: bad response\n");
		return;
	}
	// check completion code
	if (buf[1] != 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: request denied: %i\n", buf[1]);
		return;
	}
	if (buf[3] != 1)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenSocks: relay address is not IPV4: %i\n", buf[3]);
		return;
	}
	((struct sockaddr_in *)&socksRelayAddr)->sin_family      = AF_INET;
	((struct sockaddr_in *)&socksRelayAddr)->sin_addr.s_addr = *(int *)&buf[4];
	((struct sockaddr_in *)&socksRelayAddr)->sin_port        = *(short *)&buf[8];
	Com_Memset(((struct sockaddr_in *)&socksRelayAddr)->sin_zero, 0, 8);

	usingSocks = qtrue;
}

/**
 * @brief NET_AddLocalAddress
 * @param[in] ifname
 * @param[in] addr
 * @param[in] netmask
 */
static void NET_AddLocalAddress(const char *ifname, struct sockaddr *addr, struct sockaddr *netmask)
{
	sa_family_t family;

	// only add addresses that have all required info.
	if (!addr || !netmask || !ifname)
	{
		return;
	}

	family = addr->sa_family;

	if (numIP < MAX_IPS)
	{
		size_t addrlen;

		if (family == AF_INET)
		{
			addrlen             = sizeof(struct sockaddr_in);
			localIP[numIP].type = NA_IP;
		}
#ifdef FEATURE_IPV6
		else if (family == AF_INET6)
		{
			addrlen             = sizeof(struct sockaddr_in6);
			localIP[numIP].type = NA_IP6;
		}
#endif
		else
		{
			return;
		}

		Q_strncpyz(localIP[numIP].ifname, ifname, sizeof(localIP[numIP].ifname));

		localIP[numIP].family = family;

		Com_Memcpy(&localIP[numIP].addr, addr, addrlen);
		Com_Memcpy(&localIP[numIP].netmask, netmask, addrlen);

		numIP++;
	}
	else
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_AddLocalAddress - numIP >= MAX_IPS\n");
	}
}

#if defined(__linux__) || defined(__APPLE__) || defined(__BSD__)

/**
 * @brief NET_GetLocalAddress
 */
static void NET_GetLocalAddress(void)
{
	struct ifaddrs *ifap, *search;

	if (getifaddrs(&ifap))
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_GetLocalAddress: Unable to get list of network interfaces: %s\n", NET_ErrorString());
	}
	else
	{
		for (search = ifap; search; search = search->ifa_next)
		{
			// Only add interfaces that are up.
			if (ifap->ifa_flags & IFF_UP)
			{
				NET_AddLocalAddress(search->ifa_name, search->ifa_addr, search->ifa_netmask);
			}
		}

		freeifaddrs(ifap);

		Sys_ShowIP();
	}
}
#else

/**
 * @brief NET_GetLocalAddress
 */
static void NET_GetLocalAddress(void)
{
	char            hostname[256];
	struct addrinfo hint;
	struct addrinfo *res = NULL;

	if (gethostname(hostname, 256) == SOCKET_ERROR)
	{
		return;
	}

	Com_Printf("Hostname: %s\n", hostname);

	Com_Memset(&hint, 0, sizeof(hint));

	hint.ai_family   = AF_UNSPEC;
	hint.ai_socktype = SOCK_DGRAM;

	if (!getaddrinfo(hostname, NULL, &hint, &res))
	{
		struct sockaddr_in mask4;
#ifdef FEATURE_IPV6
		struct sockaddr_in6 mask6;
#endif
		struct addrinfo *search;

		// On operating systems where it's more difficult to find out the configured interfaces,
		// we'll just assume a netmask with all bits set.

		Com_Memset(&mask4, 0, sizeof(mask4));
#ifdef FEATURE_IPV6
		Com_Memset(&mask6, 0, sizeof(mask6));
#endif
		mask4.sin_family = AF_INET;
		Com_Memset(&mask4.sin_addr.s_addr, 0xFF, sizeof(mask4.sin_addr.s_addr));
#ifdef FEATURE_IPV6
		mask6.sin6_family = AF_INET6;
		Com_Memset(&mask6.sin6_addr, 0xFF, sizeof(mask6.sin6_addr));
#endif
		// add all IPs from returned list.
		for (search = res; search; search = search->ai_next)
		{
			if (search->ai_family == AF_INET)
			{
				NET_AddLocalAddress("", search->ai_addr, (struct sockaddr *) &mask4);
			}
#ifdef FEATURE_IPV6
			else if (search->ai_family == AF_INET6)
			{
				NET_AddLocalAddress("", search->ai_addr, (struct sockaddr *) &mask6);
			}
#endif
		}

		Sys_ShowIP();
	}

	if (res)
	{
		freeaddrinfo(res);
	}
}
#endif

/**
 * @brief NET_OpenIP
 */
void NET_OpenIP(void)
{
	int i;
	int err;
	int port = net_port->integer;
#ifdef FEATURE_IPV6
	int port6 = net_port6->integer;
#endif

#ifndef __ANDROID__
	NET_GetLocalAddress();
#endif

	// automatically scan for a valid port, so multiple
	// dedicated servers can be started without requiring
	// a different net_port for each one

#ifdef FEATURE_IPV6
	if (net_enabled->integer & NET_ENABLEV6)
	{
		for (i = 0 ; i < 10 ; i++)
		{
			ip6_socket = NET_IP6Socket(net_ip6->string, port6 + i, &boundto, &err);
			if (ip6_socket != INVALID_SOCKET)
			{
				Cvar_SetValue("net_port6", port6 + i);
				break;
			}
			else
			{
				if (err == EAFNOSUPPORT)
				{
					break;
				}
			}
		}
		if (ip6_socket == INVALID_SOCKET)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenIP - Couldn't bind to a v6 ip address\n");
		}
	}
#endif

	if (net_enabled->integer & NET_ENABLEV4)
	{
		for (i = 0 ; i < 10 ; i++)
		{
			ip_socket = NET_IPSocket(net_ip->string, port + i, &err);
			if (ip_socket != INVALID_SOCKET)
			{
				Cvar_SetValue("net_port", port + i);

				if (net_socksEnabled->integer)
				{
					NET_OpenSocks(port + i);
				}

				break;
			}
			else
			{
				if (err == EAFNOSUPPORT)
				{
					break;
				}
			}
		}

		if (ip_socket == INVALID_SOCKET)
		{
			Com_Printf(S_COLOR_YELLOW "WARNING: NET_OpenIP - Couldn't bind to a v4 ip address\n");
		}
	}
}

//===================================================================

/**
 * @brief NET_GetCvars
 * @return
 */
static qboolean NET_GetCvars(void)
{
	int modified;

#ifdef DEDICATED
	// I want server owners to explicitly turn on ipv6 support.
	net_enabled = Cvar_Get("net_enabled", "1", CVAR_LATCH | CVAR_ARCHIVE);
#else

#ifdef FEATURE_IPV6
	// End users have it enabled so they can connect to ipv6-only hosts, but ipv4 will be used if available due to ping
	net_enabled = Cvar_Get("net_enabled", "3", CVAR_LATCH | CVAR_ARCHIVE);
#else
	net_enabled = Cvar_Get("net_enabled", "1", CVAR_LATCH | CVAR_ARCHIVE);
#endif // FEATURE_IPV6

#endif // DEDICATED
	modified              = net_enabled->modified;
	net_enabled->modified = qfalse;

	net_ip           = Cvar_Get("net_ip", "0.0.0.0", CVAR_LATCH);
	modified        += net_ip->modified;
	net_ip->modified = qfalse;

#ifdef FEATURE_IPV6
	net_ip6           = Cvar_Get("net_ip6", "::", CVAR_LATCH);
	modified         += net_ip6->modified;
	net_ip6->modified = qfalse;
#endif

	net_port           = Cvar_Get("net_port", va("%i", PORT_SERVER), CVAR_LATCH);
	modified          += net_port->modified;
	net_port->modified = qfalse;

#ifdef FEATURE_IPV6
	net_port6           = Cvar_Get("net_port6", va("%i", PORT_SERVER), CVAR_LATCH);
	modified           += net_port6->modified;
	net_port6->modified = qfalse;

	// Some cvars for configuring multicast options which facilitates scanning for servers on local subnets.
	net_mcast6addr           = Cvar_Get("net_mcast6addr", NET_MULTICAST_IP6, CVAR_LATCH | CVAR_ARCHIVE);
	modified                += net_mcast6addr->modified;
	net_mcast6addr->modified = qfalse;

#ifdef _WIN32
	net_mcast6iface = Cvar_Get("net_mcast6iface", "0", CVAR_LATCH | CVAR_ARCHIVE);
#else
	net_mcast6iface = Cvar_Get("net_mcast6iface", "", CVAR_LATCH | CVAR_ARCHIVE);
#endif //  _WIN32
	modified                 += net_mcast6iface->modified;
	net_mcast6iface->modified = qfalse;
#endif // FEATURE_IPV6

	net_socksEnabled           = Cvar_Get("net_socksEnabled", "0", CVAR_LATCH | CVAR_ARCHIVE);
	modified                  += net_socksEnabled->modified;
	net_socksEnabled->modified = qfalse;

	net_socksServer           = Cvar_Get("net_socksServer", "", CVAR_LATCH | CVAR_ARCHIVE);
	modified                 += net_socksServer->modified;
	net_socksServer->modified = qfalse;

	net_socksPort           = Cvar_Get("net_socksPort", "1080", CVAR_LATCH | CVAR_ARCHIVE);
	modified               += net_socksPort->modified;
	net_socksPort->modified = qfalse;

	net_socksUsername           = Cvar_Get("net_socksUsername", "", CVAR_LATCH | CVAR_ARCHIVE);
	modified                   += net_socksUsername->modified;
	net_socksUsername->modified = qfalse;

	net_socksPassword           = Cvar_Get("net_socksPassword", "", CVAR_LATCH | CVAR_ARCHIVE);
	modified                   += net_socksPassword->modified;
	net_socksPassword->modified = qfalse;

	net_dropsim = Cvar_Get("net_dropsim", "0", CVAR_TEMP | CVAR_CHEAT);

	return modified ? qtrue : qfalse;
}

/**
 * @brief NET_Config
 * @param[in] enableNetworking
 */
static void NET_Config(qboolean enableNetworking)
{
	qboolean modified;
	qboolean stop;
	qboolean start;

	// get any latched changes to cvars
	modified = NET_GetCvars();

	if (!net_enabled->integer)
	{
		enableNetworking = 0;
	}

	// if enable state is the same and no cvars were modified, we have nothing to do
	if (enableNetworking == networkingEnabled && !modified)
	{
		Com_Printf("Network not modified\n");
		return;
	}

	if (enableNetworking == networkingEnabled)
	{
		if (enableNetworking)
		{
			stop  = qtrue;
			start = qtrue;
		}
		else
		{
			stop  = qfalse;
			start = qfalse;
		}
	}
	else
	{
		if (enableNetworking)
		{
			stop  = qfalse;
			start = qtrue;
		}
		else
		{
			stop  = qtrue;
			start = qfalse;
		}
		networkingEnabled = enableNetworking;
	}

	if (stop)
	{
		if (ip_socket != INVALID_SOCKET)
		{
			closesocket(ip_socket);
			ip_socket = INVALID_SOCKET;
		}

#ifdef FEATURE_IPV6
		if (multicast6_socket != INVALID_SOCKET)
		{
			if (multicast6_socket != ip6_socket)
			{
				closesocket(multicast6_socket);
			}

			multicast6_socket = INVALID_SOCKET;
		}

		if (ip6_socket != INVALID_SOCKET)
		{
			closesocket(ip6_socket);
			ip6_socket = INVALID_SOCKET;
		}
#endif

		if (socks_socket != INVALID_SOCKET)
		{
			closesocket(socks_socket);
			socks_socket = INVALID_SOCKET;
		}

		Com_Printf("Network shutdown\n");
	}

	if (start)
	{
		if (net_enabled->integer)
		{
			NET_OpenIP();
#ifdef FEATURE_IPV6
			NET_SetMulticast6();
#endif
		}
		Com_Printf("Network initialized\n");
	}
}

/**
 * @brief NET_Init
 */
void NET_Init(void)
{
#ifdef _WIN32
	int sock_ret;

	sock_ret = WSAStartup(MAKEWORD(1, 1), &winsockdata);
	if (sock_ret != 0)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: NET_Init: Winsock initialization failed, returned %d\n", sock_ret);
		return;
	}

	winsockInitialized = qtrue;
	Com_Printf("Winsock initialized\n");
#endif

	NET_Config(qtrue);

	Cmd_AddCommand("net_restart", NET_Restart_f, "Restarts the network.");
}

/**
 * @brief NET_Shutdown
 */
void NET_Shutdown(void)
{
	if (!networkingEnabled)
	{
		return;
	}

	NET_Config(qfalse);

#ifdef _WIN32
	WSACleanup();
	winsockInitialized = qfalse;
#endif
}

/**
 * @brief Called from NET_Sleep which uses select() to determine which sockets have seen action.
 * @param fdr
 */
void NET_Event(fd_set *fdr)
{
	byte     bufData[MAX_MSGLEN + 1];
	netadr_t from = { 0, { 0 }, { 0 }, 0, 0 };
	msg_t    netmsg;

	while (1)
	{
		MSG_Init(&netmsg, bufData, sizeof(bufData));

		if (NET_GetPacket(&from, &netmsg, fdr))
		{
			if (net_dropsim->value > 0.0f && net_dropsim->value <= 100.0f)
			{
				// net_dropsim->value percent of incoming packets get dropped.
				if (rand() < (int)(((double)RAND_MAX) / 100.0 * (double)net_dropsim->value))
				{
					continue;          // drop this packet
				}
			}

			if (com_sv_running->integer)
			{
				Com_RunAndTimeServerPacket(&from, &netmsg);
			}
			else
			{
				CL_PacketEvent(from, &netmsg);
			}
		}
		else
		{
			break;
		}
	}
}

/**
 * @brief Sleeps msec or until something happens on the network
 * @param[in] msec
 */
void NET_Sleep(int msec)
{
	struct timeval timeout;
	fd_set         fdset;
	int            retval;
	SOCKET         highestfd = INVALID_SOCKET;

	if (msec < 0)
	{
		msec = 0;
	}

	FD_ZERO(&fdset);

	if (ip_socket != INVALID_SOCKET)
	{
		FD_SET(ip_socket, &fdset);
		highestfd = ip_socket;
	}
#ifdef FEATURE_IPV6
	if (ip6_socket != INVALID_SOCKET)
	{
		FD_SET(ip6_socket, &fdset);
		if (highestfd == INVALID_SOCKET || ip6_socket > highestfd)
		{
			highestfd = ip6_socket;
		}
	}
#endif

#ifdef _WIN32
	if (highestfd == INVALID_SOCKET)
	{
		// windows ain't happy when select is called without valid FDs
		SleepEx(msec, 0);
		return;
	}
#endif

	timeout.tv_sec  = msec / 1000;
	timeout.tv_usec = (msec % 1000) * 1000;
	retval          = select(highestfd + 1, &fdset, NULL, NULL, &timeout);

	if (retval == SOCKET_ERROR)
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: select() syscall failed: %s\n", NET_ErrorString());
	}
	else if (retval > 0)
	{
		NET_Event(&fdset);
	}
}

/**
 * @brief NET_Restart_f
 */
void NET_Restart_f(void)
{
	NET_Config(networkingEnabled);
}
