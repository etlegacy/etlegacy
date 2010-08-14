/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include <math.h>

Q3DEF_BEGIN
	#include "../client/client.h"
	#include "mac_local.h"
Q3DEF_END

/*
#ifndef DEDICATED
#include "GameRanger SDK/GameRanger.h"
#endif
*/

static qboolean gOTInited;
static EndpointRef endpoint = kOTInvalidEndpointRef;

#define MAX_IPS     16
static int num_interfaces = 0;
static InetInterfaceInfo sys_inetInfo[MAX_IPS];
static byte localIP[MAX_IPS][4];

InetSvcRef inet_services;

static TUDErr uderr;

OTClientContextPtr clientContext = NULL;

void RcvUDErr( EndpointRef inEndpoint ) {
	memset( &uderr, 0, sizeof( uderr ) );
	uderr.addr.maxlen = 0;
	uderr.opt.maxlen = 0;
	OTRcvUDErr( inEndpoint, &uderr );
}

void HandleOTError( int err, const char *func, EndpointRef inEndpoint ) {
	int r;
	static int lastErr;

	if ( err != lastErr ) {
		Com_Printf( "%s: error %i\n", func, err );
	}

	// if we don't call OTLook, things wedge
	r = OTLook( inEndpoint );
	if ( err != lastErr ) {
		Com_DPrintf( "%s: OTLook %i\n", func, r );
	}

	switch ( r )
	{
	case T_UDERR:
		RcvUDErr( inEndpoint );
		if ( err != lastErr ) {
			Com_DPrintf( "%s: OTRcvUDErr %i\n", func, uderr.error );
		}
		break;
	default:
//			Com_Printf( "%s: Unknown OTLook error %i\n", func, r );
		break;
	}
	lastErr = err;  // don't spew tons of messages
}

/*
=================
NotifyProc
=================
*/
pascal void NotifyProc( void* contextPtr, OTEventCode code,
						OTResult result, void* cookie ) {
	switch ( code )
	{
	case T_OPENCOMPLETE:
		endpoint = (EndpointRef) cookie;
		break;
	case T_UDERR:
		RcvUDErr( (EndpointRef)cookie );
		break;
	}
}


/*
=================
GetFourByteOption
=================
*/
static OTResult GetFourByteOption( EndpointRef ep,
								   OTXTILevel level,
								   OTXTIName name,
								   UInt32   *value ) {
	OTResult err;
	TOption option;
	TOptMgmt request;
	TOptMgmt result;

	/* Set up the option buffer */
	option.len  = kOTFourByteOptionSize;
	option.level = level;
	option.name = name;
	option.status = 0;
	option.value[0] = 0; // Ignored because we're getting the value.

	/* Set up the request parameter for OTOptionManagement to point
	to the option buffer we just filled out */

	request.opt.buf = (UInt8 *) &option;
	request.opt.len = sizeof( option );
	request.flags = T_CURRENT;

	/* Set up the reply parameter for OTOptionManagement. */
	result.opt.buf  = (UInt8 *) &option;
	result.opt.maxlen = sizeof( option );

	err = OTOptionManagement( ep, &request, &result );

	if ( err == noErr ) {
		switch ( option.status )
		{
		case T_SUCCESS:
		case T_READONLY:
			*value = option.value[0];
			break;
		default:
			err = option.status;
			break;
		}
	}

	return ( err );
}


/*
=================
SetFourByteOption
=================
*/
static OTResult SetFourByteOption( EndpointRef ep,
								   OTXTILevel level,
								   OTXTIName name,
								   UInt32 value ) {
	OTResult err;
	TOption option;
	TOptMgmt request;
	TOptMgmt result;

	/* Set up the option buffer to specify the option and value to set. */
	option.len  = kOTFourByteOptionSize;
	option.level = level;
	option.name = name;
	option.status = 0;
	option.value[0] = value;

	/* Set up request parameter for OTOptionManagement */
	request.opt.buf = (UInt8 *) &option;
	request.opt.len = sizeof( option );
	request.flags  = T_NEGOTIATE;

	/* Set up reply parameter for OTOptionManagement. */
	result.opt.buf  = (UInt8 *) &option;
	result.opt.maxlen  = sizeof( option );


	err = OTOptionManagement( ep, &request, &result );

	if ( err == noErr ) {
		if ( option.status != T_SUCCESS ) {
			err = option.status;
		}
	}

	return ( err );
}


/*
==================
Sys_ShowIP
==================
*/
void Sys_ShowIP( void ) {
	int i;

	for ( i = 0; i < num_interfaces; i++ ) {
		Com_Printf( "IP: %i.%i.%i.%i\n", localIP[i][0], localIP[i][1], localIP[i][2], localIP[i][3] );
	}
}

/*
=====================
NET_GetLocalAddress
=====================
*/
void NET_GetLocalAddress( void ) {
	OSStatus err;
	int iOT;
	InetInterfaceInfo inetInfo;

	num_interfaces = 0;
	iOT = 0;

	while ( true ) {
		err = OTInetGetInterfaceInfo( &inetInfo, iOT );
		if ( err ) {
			break;
		}

		if ( inetInfo.fAddress != 0 ) {
			sys_inetInfo[num_interfaces] = inetInfo;
			UInt32 IP = ntohl( inetInfo.fAddress );
			localIP[num_interfaces][0] = ( (byte*)&IP )[0];
			localIP[num_interfaces][1] = ( (byte*)&IP )[1];
			localIP[num_interfaces][2] = ( (byte*)&IP )[2];
			localIP[num_interfaces][3] = ( (byte*)&IP )[3];
			Com_Printf( "LocalAddress: %i.%i.%i.%i\n",
						localIP[ num_interfaces ][0],
						localIP[ num_interfaces ][1],
						localIP[ num_interfaces ][2],
						localIP[ num_interfaces ][3] );
			UInt32 netmask = ntohl( inetInfo.fNetmask );
			Com_Printf( "Netmask: %i.%i.%i.%i\n",
						( (byte*)&netmask )[0],
						( (byte*)&netmask )[1],
						( (byte*)&netmask )[2],
						( (byte*)&netmask )[3] );
			num_interfaces++;
		}

		iOT++;
	}
}


/*
==================
Sys_InitNetworking


struct InetAddress
{
		OTAddressType	fAddressType;	// always AF_INET
		InetPort		fPort;			// Port number
		InetHost		fHost;			// Host address in net byte order
		UInt8			fUnused[8];		// Traditional unused bytes
};
typedef struct InetAddress InetAddress;

==================
*/
void Sys_InitNetworking( void ) {
	cvar_t  *ip;
	int port;

	OSStatus err;
	OTConfigurationRef config;
	TBind bind, bindOut;
	InetAddress in, out;
	int i;

	ip = Cvar_Get( "net_ip", "localhost", CVAR_LATCH );
	port = Cvar_Get( "net_port", va( "%i", PORT_SERVER ), CVAR_LATCH )->integer;

	Com_Printf( "----- Sys_InitNetworking -----\n" );
	// init OpenTransport
	Com_Printf( "... InitOpenTransport()\n" );

	if ( (Ptr) kUnresolvedCFragSymbolAddress == (Ptr) InitOpenTransportInContext ) {
		Com_Error( ERR_FATAL, "OpenTransport extensions not installed" );
		return;
	}

	err = InitOpenTransportInContext( kInitOTForApplicationMask, NULL /*&clientContext*/ );
	if ( err != noErr ) {
		Com_Printf( "InitOpenTransport() failed\n" );
		Com_Printf( "------------------------------\n" );
		return;
	}

	gOTInited = qtrue;

	// get an endpoint
	Com_Printf( "... OTOpenEndpoint()\n" );
	config = OTCreateConfiguration( kUDPName );

	endpoint = OTOpenEndpointInContext( config, 0, NULL, &err, NULL );

	if ( err != noErr ) {
		endpoint = 0;
		Com_Printf( "OTOpenEndpoint() failed\n" );
		Com_Printf( "------------------------------\n" );
		return;
	}

	// set non-blocking
	err = OTSetNonBlocking( endpoint );

	// scan for a valid port in our range
	Com_Printf( "... OTBind()\n" );
	for ( i = 0 ; i < 10 ; i++ )
	{
		in.fAddressType = AF_INET;
		in.fPort = port + i;
		in.fHost = 0;

		bind.addr.maxlen = sizeof( in );
		bind.addr.len = sizeof( in );
		bind.addr.buf = (unsigned char *)&in;
		bind.qlen = 0;

		bindOut.addr.maxlen = sizeof( out );
		bindOut.addr.len = sizeof( out );
		bindOut.addr.buf = (unsigned char *)&out;
		bindOut.qlen = 0;

		err = OTBind( endpoint, &bind, &bindOut );
		if ( err == noErr ) {
			Com_Printf( "Opened UDP endpoint at port %i\n",
						out.fPort );
			break;
		}
	}

	if ( err != noErr ) {
		Com_Printf( "Couldn't bind a local port\n" );
	}

	// get the local address for LAN client detection
	NET_GetLocalAddress();

	// set to allow broadcasts
	err = SetFourByteOption( endpoint, INET_IP, kIP_BROADCAST, T_YES );

	if ( err != noErr ) {
		Com_Printf( "IP_BROADCAST failed\n" );
	}

	inet_services = OTOpenInternetServicesInContext( kDefaultInternetServicesPath, 0, &err, NULL );

	/*
#ifndef DEDICATED
	if (GRIsHostCmd())
		GRHostReady();
#endif
	*/

	Com_Printf( "------------------------------\n" );
}


/*
==================
Sys_ShutdownNetworking
==================
*/
void Sys_ShutdownNetworking( void ) {
	Com_Printf( "Sys_ShutdownNetworking();\n" );

	/*
#ifndef DEDICATED
	if (GRIsHostCmd())
		GRHostClosed();
#endif
	*/

	if ( endpoint != kOTInvalidEndpointRef ) {
		OTUnbind( endpoint );
		OTCloseProvider( endpoint );
		endpoint = kOTInvalidEndpointRef;
	}
	if ( inet_services ) {
		OTCloseProvider( inet_services );
	}
	if ( gOTInited ) {
		CloseOpenTransportInContext( NULL );
		gOTInited = qfalse;
	}
}

/*
====================
NET_Shutdown
====================
*/
void    NET_Shutdown( void ) {

}

/*
=============
Sys_StringToAdr

Does NOT parse port numbers
=============
*/
qboolean Sys_StringToAdr( const char *s, netadr_t *a ) {
	OSStatus err;
	InetHostInfo info;

	err = OTInetStringToAddress( inet_services, (char *)s, &info );
	if ( err ) {
		char name[256];
		Com_DPrintf( "Sys_StringToAdr addr failed: %s\n", s );
		return qfalse;
	}

	a->type = NA_IP;
	*(int *)a->ip = ntohl( info.addrs[0] );
	a->port = 0;

	return qtrue;
}

/*
==================
Sys_SendPacket
==================
*/
#define MAX_PACKETLEN   1400
void Sys_SendPacket( int length, const void *data, netadr_t to ) {
	TUnitData d;
	InetAddress inAddr;
	OSStatus err;

	if ( !endpoint ) {
		return;
	}

	if ( length > MAX_PACKETLEN ) {
		Com_Error( ERR_DROP, "Sys_SendPacket: length > MAX_PACKETLEN" );
	}

	inAddr.fAddressType = AF_INET;
	// netadr_t holds port in network order
	// ?but it seems we have to give it in host order here?
	inAddr.fPort = ntohs( to.port );
	if ( to.type == NA_BROADCAST ) {
		inAddr.fHost = -1;
	} else {
		inAddr.fHost = htonl( *(int*)&to.ip );
	}

	memset( &d, 0, sizeof( d ) );

	d.addr.len = sizeof( inAddr );
	d.addr.maxlen = sizeof( inAddr );
	d.addr.buf = (unsigned char *)&inAddr;

	d.opt.len = 0;
	d.opt.maxlen = 0;
	d.opt.buf = NULL;

	d.udata.len = length;
	d.udata.maxlen = length;
	d.udata.buf = (unsigned char *)data;

	err = OTSndUData( endpoint, &d );
	if ( err ) {
		HandleOTError( err, "Sys_SendPacket", endpoint );
	}
}

/*
==================
Sys_GetPacket

Never called by the game logic, just the system event queing
==================
*/
qboolean    Sys_GetPacket( netadr_t *net_from, msg_t *net_message ) {
	TUnitData d;
	InetAddress inAddr;
	OSStatus err;
	OTFlags flags;

	if ( !endpoint ) {
		return qfalse;
	}

	inAddr.fAddressType = AF_INET;
	inAddr.fPort = 0;
	inAddr.fHost = 0;

	memset( &d, 0, sizeof( d ) );

	d.addr.len = sizeof( inAddr );
	d.addr.maxlen = sizeof( inAddr );
	d.addr.buf = (unsigned char *)&inAddr;

	d.opt.len = 0;
	d.opt.maxlen = 0;
	d.opt.buf = 0;

	d.udata.len = net_message->maxsize;
	d.udata.maxlen = net_message->maxsize;
	d.udata.buf = net_message->data;

	err = OTRcvUData( endpoint, &d, &flags );
	if ( err ) {
		if ( err == kOTNoDataErr ) {
			return qfalse;
		}
		HandleOTError( err, "Sys_GetPacket", endpoint );
		return qfalse;
	}

	net_from->type = NA_IP;
	// the game wants them in network order, for some reason we're getting them in host order
	net_from->port = htons( inAddr.fPort );
	*(int *)net_from->ip = ntohl( inAddr.fHost );

	net_message->cursize = d.udata.len;

	return qtrue;
}


/*
==================
Sys_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
qboolean    Sys_IsLANAddress( netadr_t adr ) {
	int i;

	if ( adr.type == NA_LOOPBACK ) {
		return qtrue;
	}

	if ( adr.type == NA_IPX ) {
		return qtrue;
	}

	if ( adr.type != NA_IP ) {
		return qfalse;
	}

	//bani
	if ( num_interfaces ) {
		unsigned long *p_ip;
		unsigned long ip;
		p_ip = (unsigned long *)&adr.ip[0];
		ip = htonl( *p_ip );

		for ( i = 0; i < num_interfaces; i++ ) {
			if ( ( sys_inetInfo[i].fAddress & sys_inetInfo[i].fNetmask ) == ( ip & sys_inetInfo[i].fNetmask ) ) {
				return qtrue;
			}
		}
		return qfalse;
	}

	// choose which comparison to use based on the class of the address being tested
	// any local adresses of a different class than the address being tested will fail based on the first byte

	if ( adr.ip[0] == 127 && adr.ip[1] == 0 && adr.ip[2] == 0 && adr.ip[3] == 1 ) {
		return qtrue;
	}

	// Class A
	if ( ( adr.ip[0] & 0x80 ) == 0x00 ) {
		for ( i = 0 ; i < num_interfaces ; i++ ) {
			if ( adr.ip[0] == localIP[i][0] ) {
				return qtrue;
			}
		}
		// the RFC1918 class a block will pass the above test
		return qfalse;
	}

	// Class B
	if ( ( adr.ip[0] & 0xc0 ) == 0x80 ) {
		for ( i = 0 ; i < num_interfaces ; i++ ) {
			if ( adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] ) {
				return qtrue;
			}
			// also check against the RFC1918 class b blocks
			if ( adr.ip[0] == 172 && localIP[i][0] == 172 && ( adr.ip[1] & 0xf0 ) == 16 && ( localIP[i][1] & 0xf0 ) == 16 ) {
				return qtrue;
			}
		}
		return qfalse;
	}

	// Class C
	for ( i = 0 ; i < num_interfaces ; i++ ) {
		if ( adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] && adr.ip[2] == localIP[i][2] ) {
			return qtrue;
		}
		// also check against the RFC1918 class c blocks
		if ( adr.ip[0] == 192 && localIP[i][0] == 192 && adr.ip[1] == 168 && localIP[i][1] == 168 ) {
			return qtrue;
		}
	}
	return qfalse;
}

void NET_Init( void ) {
}

void NET_Sleep( int i ) {
}

extern "C" int WSACleanup( void ) {
	return 0;
}
