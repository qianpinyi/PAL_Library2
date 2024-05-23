/*
  SDL_net:  An example cross-platform network library for use with SDL
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>
  Copyright (C) 2012 Simeon Maxein <smaxein@googlemail.com>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* $Id$ */

#ifndef _SDL_NET_H
#define _SDL_NET_H

#ifdef WITHOUT_SDL
#include <stdint.h>
typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;

typedef struct SDLNet_version {
    Uint8 major;
    Uint8 minor;
    Uint8 patch;
} SDLNet_version;

#define SDL_vsnprintf vsnprintf
#define SDL_memcpy memcpy
#define SDL_malloc malloc
#define SDL_realloc realloc
#define SDL_free free
#define SDL_memset memset

#else /* WITHOUT_SDL */

#include "SDL.h"
#include "SDL_endian.h"
#include "SDL_version.h"

typedef SDL_version SDLNet_version;

#endif /* WITHOUT_SDL */

#include "begin_code.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Printable format: "%d.%d.%d", MAJOR, MINOR, PATCHLEVEL
*/
#define SDL_NET_MAJOR_VERSION   2
#define SDL_NET_MINOR_VERSION   0
#define SDL_NET_PATCHLEVEL      1

/* This macro can be used to fill a version structure with the compile-time
 * version of the SDL_net library.
 */
#define SDL_NET_VERSION(X)                          \
{                                                   \
    (X)->major = SDL_NET_MAJOR_VERSION;             \
    (X)->minor = SDL_NET_MINOR_VERSION;             \
    (X)->patch = SDL_NET_PATCHLEVEL;                \
}

/* This function gets the version of the dynamically linked SDL_net library.
   it should NOT be used to fill a version structure, instead you should
   use the SDL_NET_VERSION() macro.
 */
extern DECLSPEC const SDLNet_version * SDLCALL SDLNet_Linked_Version(void);

/* Initialize/Cleanup the network API
   SDL must be initialized before calls to functions in this library,
   because this library uses utility functions from the SDL library.
*/
extern DECLSPEC int  SDLCALL SDLNet_Init(void);
extern DECLSPEC void SDLCALL SDLNet_Quit(void);

/***********************************************************************/
/* IPv4 hostname resolution API                                        */
/***********************************************************************/

typedef struct {
    Uint32 host;            /* 32-bit IPv4 host address */
    Uint16 port;            /* 16-bit protocol port */
} IPaddress;

/* Resolve a host name and port to an IP address in network form.
   If the function succeeds, it will return 0.
   If the host couldn't be resolved, the host portion of the returned
   address will be INADDR_NONE, and the function will return -1.
   If 'host' is NULL, the resolved host will be set to INADDR_ANY.
 */
#ifndef INADDR_ANY
#define INADDR_ANY      0x00000000
#endif
#ifndef INADDR_NONE
#define INADDR_NONE     0xFFFFFFFF
#endif
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK     0x7f000001
#endif
#ifndef INADDR_BROADCAST
#define INADDR_BROADCAST    0xFFFFFFFF
#endif
extern DECLSPEC int SDLCALL SDLNet_ResolveHost(IPaddress *address, const char *host, Uint16 port);

/* Resolve an ip address to a host name in canonical form.
   If the ip couldn't be resolved, this function returns NULL,
   otherwise a pointer to a static buffer containing the hostname
   is returned.  Note that this function is not thread-safe.
*/
extern DECLSPEC const char * SDLCALL SDLNet_ResolveIP(const IPaddress *ip);

/* Get the addresses of network interfaces on this system.
   This returns the number of addresses saved in 'addresses'
 */
extern DECLSPEC int SDLCALL SDLNet_GetLocalAddresses(IPaddress *addresses, int maxcount);

/***********************************************************************/
/* TCP network API                                                     */
/***********************************************************************/

typedef struct _TCPsocket *TCPsocket;

/* Open a TCP network socket
   If ip.host is INADDR_NONE or INADDR_ANY, this creates a local server
   socket on the given port, otherwise a TCP connection to the remote
   host and port is attempted. The address passed in should already be
   swapped to network byte order (addresses returned from
   SDLNet_ResolveHost() are already in the correct form).
   The newly created socket is returned, or NULL if there was an error.
*/
extern DECLSPEC TCPsocket SDLCALL SDLNet_TCP_Open(IPaddress *ip);

/* Accept an incoming connection on the given server socket.
   The newly created socket is returned, or NULL if there was an error.
*/
extern DECLSPEC TCPsocket SDLCALL SDLNet_TCP_Accept(TCPsocket server);

/* Get the IP address of the remote system associated with the socket.
   If the socket is a server socket, this function returns NULL.
*/
extern DECLSPEC IPaddress * SDLCALL SDLNet_TCP_GetPeerAddress(TCPsocket sock);

/* Send 'len' bytes of 'data' over the non-server socket 'sock'
   This function returns the actual amount of data sent.  If the return value
   is less than the amount of data sent, then either the remote connection was
   closed, or an unknown socket error occurred.
*/
extern DECLSPEC int SDLCALL SDLNet_TCP_Send(TCPsocket sock, const void *data,
        int len);

/* Receive up to 'maxlen' bytes of data over the non-server socket 'sock',
   and store them in the buffer pointed to by 'data'.
   This function returns the actual amount of data received.  If the return
   value is less than or equal to zero, then either the remote connection was
   closed, or an unknown socket error occurred.
*/
extern DECLSPEC int SDLCALL SDLNet_TCP_Recv(TCPsocket sock, void *data, int maxlen);

/* Close a TCP network socket */
extern DECLSPEC void SDLCALL SDLNet_TCP_Close(TCPsocket sock);


/***********************************************************************/
/* UDP network API                                                     */
/***********************************************************************/

/* The maximum channels on a a UDP socket */
#define SDLNET_MAX_UDPCHANNELS  32
/* The maximum addresses bound to a single UDP socket channel */
#define SDLNET_MAX_UDPADDRESSES 4

typedef struct _UDPsocket *UDPsocket;
typedef struct {
    int channel;        /* The src/dst channel of the packet */
    Uint8 *data;        /* The packet data */
    int len;            /* The length of the packet data */
    int maxlen;         /* The size of the data buffer */
    int status;         /* packet status after sending */
    IPaddress address;  /* The source/dest address of an incoming/outgoing packet */
} UDPpacket;

/* Allocate/resize/free a single UDP packet 'size' bytes long.
   The new packet is returned, or NULL if the function ran out of memory.
 */
extern DECLSPEC UDPpacket * SDLCALL SDLNet_AllocPacket(int size);
extern DECLSPEC int SDLCALL SDLNet_ResizePacket(UDPpacket *packet, int newsize);
extern DECLSPEC void SDLCALL SDLNet_FreePacket(UDPpacket *packet);

/* Allocate/Free a UDP packet vector (array of packets) of 'howmany' packets,
   each 'size' bytes long.
   A pointer to the first packet in the array is returned, or NULL if the
   function ran out of memory.
 */
extern DECLSPEC UDPpacket ** SDLCALL SDLNet_AllocPacketV(int howmany, int size);
extern DECLSPEC void SDLCALL SDLNet_FreePacketV(UDPpacket **packetV);


/* Open a UDP network socket
   If 'port' is non-zero, the UDP socket is bound to a local port.
   The 'port' should be given in native byte order, but is used
   internally in network (big endian) byte order, in addresses, etc.
   This allows other systems to send to this socket via a known port.
*/
extern DECLSPEC UDPsocket SDLCALL SDLNet_UDP_Open(Uint16 port);

/* Set the percentage of simulated packet loss for packets sent on the socket.
*/
extern DECLSPEC void SDLCALL SDLNet_UDP_SetPacketLoss(UDPsocket sock, int percent);

/* Bind the address 'address' to the requested channel on the UDP socket.
   If the channel is -1, then the first unbound channel that has not yet
   been bound to the maximum number of addresses will be bound with
   the given address as it's primary address.
   If the channel is already bound, this new address will be added to the
   list of valid source addresses for packets arriving on the channel.
   If the channel is not already bound, then the address becomes the primary
   address, to which all outbound packets on the channel are sent.
   This function returns the channel which was bound, or -1 on error.
*/
extern DECLSPEC int SDLCALL SDLNet_UDP_Bind(UDPsocket sock, int channel, const IPaddress *address);

/* Unbind all addresses from the given channel */
extern DECLSPEC void SDLCALL SDLNet_UDP_Unbind(UDPsocket sock, int channel);

/* Get the primary IP address of the remote system associated with the
   socket and channel.  If the channel is -1, then the primary IP port
   of the UDP socket is returned -- this is only meaningful for sockets
   opened with a specific port.
   If the channel is not bound and not -1, this function returns NULL.
 */
extern DECLSPEC IPaddress * SDLCALL SDLNet_UDP_GetPeerAddress(UDPsocket sock, int channel);

/* Send a vector of packets to the the channels specified within the packet.
   If the channel specified in the packet is -1, the packet will be sent to
   the address in the 'src' member of the packet.
   Each packet will be updated with the status of the packet after it has
   been sent, -1 if the packet send failed.
   This function returns the number of packets sent.
*/
extern DECLSPEC int SDLCALL SDLNet_UDP_SendV(UDPsocket sock, UDPpacket **packets, int npackets);

/* Send a single packet to the specified channel.
   If the channel specified in the packet is -1, the packet will be sent to
   the address in the 'src' member of the packet.
   The packet will be updated with the status of the packet after it has
   been sent.
   This function returns 1 if the packet was sent, or 0 on error.

   NOTE:
   The maximum size of the packet is limited by the MTU (Maximum Transfer Unit)
   of the transport medium.  It can be as low as 250 bytes for some PPP links,
   and as high as 1500 bytes for ethernet.
*/
extern DECLSPEC int SDLCALL SDLNet_UDP_Send(UDPsocket sock, int channel, UDPpacket *packet);

/* Receive a vector of pending packets from the UDP socket.
   The returned packets contain the source address and the channel they arrived
   on.  If they did not arrive on a bound channel, the the channel will be set
   to -1.
   The channels are checked in highest to lowest order, so if an address is
   bound to multiple channels, the highest channel with the source address
   bound will be returned.
   This function returns the number of packets read from the network, or -1
   on error.  This function does not block, so can return 0 packets pending.
*/
extern DECLSPEC int SDLCALL SDLNet_UDP_RecvV(UDPsocket sock, UDPpacket **packets);

/* Receive a single packet from the UDP socket.
   The returned packet contains the source address and the channel it arrived
   on.  If it did not arrive on a bound channel, the the channel will be set
   to -1.
   The channels are checked in highest to lowest order, so if an address is
   bound to multiple channels, the highest channel with the source address
   bound will be returned.
   This function returns the number of packets read from the network, or -1
   on error.  This function does not block, so can return 0 packets pending.
*/
extern DECLSPEC int SDLCALL SDLNet_UDP_Recv(UDPsocket sock, UDPpacket *packet);

/* Close a UDP network socket */
extern DECLSPEC void SDLCALL SDLNet_UDP_Close(UDPsocket sock);


/***********************************************************************/
/* Hooks for checking sockets for available data                       */
/***********************************************************************/

typedef struct _SDLNet_SocketSet *SDLNet_SocketSet;

/* Any network socket can be safely cast to this socket type */
typedef struct _SDLNet_GenericSocket {
    int ready;
} *SDLNet_GenericSocket;

/* Allocate a socket set for use with SDLNet_CheckSockets()
   This returns a socket set for up to 'maxsockets' sockets, or NULL if
   the function ran out of memory.
 */
extern DECLSPEC SDLNet_SocketSet SDLCALL SDLNet_AllocSocketSet(int maxsockets);

/* Add a socket to a set of sockets to be checked for available data */
extern DECLSPEC int SDLCALL SDLNet_AddSocket(SDLNet_SocketSet set, SDLNet_GenericSocket sock);
SDL_FORCE_INLINE int SDLNet_TCP_AddSocket(SDLNet_SocketSet set, TCPsocket sock)
{
    return SDLNet_AddSocket(set, (SDLNet_GenericSocket)sock);
}
SDL_FORCE_INLINE int SDLNet_UDP_AddSocket(SDLNet_SocketSet set, UDPsocket sock)
{
    return SDLNet_AddSocket(set, (SDLNet_GenericSocket)sock);
}


/* Remove a socket from a set of sockets to be checked for available data */
extern DECLSPEC int SDLCALL SDLNet_DelSocket(SDLNet_SocketSet set, SDLNet_GenericSocket sock);
SDL_FORCE_INLINE int SDLNet_TCP_DelSocket(SDLNet_SocketSet set, TCPsocket sock)
{
    return SDLNet_DelSocket(set, (SDLNet_GenericSocket)sock);
}
SDL_FORCE_INLINE int SDLNet_UDP_DelSocket(SDLNet_SocketSet set, UDPsocket sock)
{
    return SDLNet_DelSocket(set, (SDLNet_GenericSocket)sock);
}

/* This function checks to see if data is available for reading on the
   given set of sockets.  If 'timeout' is 0, it performs a quick poll,
   otherwise the function returns when either data is available for
   reading, or the timeout in milliseconds has elapsed, which ever occurs
   first.  This function returns the number of sockets ready for reading,
   or -1 if there was an error with the select() system call.
*/
extern DECLSPEC int SDLCALL SDLNet_CheckSockets(SDLNet_SocketSet set, Uint32 timeout);

/* After calling SDLNet_CheckSockets(), you can use this function on a
   socket that was in the socket set, to find out if data is available
   for reading.
*/
#define SDLNet_SocketReady(sock) _SDLNet_SocketReady((SDLNet_GenericSocket)(sock))
SDL_FORCE_INLINE int _SDLNet_SocketReady(SDLNet_GenericSocket sock)
{
    return (sock != NULL) && (sock->ready);
}

/* Free a set of sockets allocated by SDL_NetAllocSocketSet() */
extern DECLSPEC void SDLCALL SDLNet_FreeSocketSet(SDLNet_SocketSet set);

/***********************************************************************/
/* Error reporting functions                                           */
/***********************************************************************/

extern DECLSPEC void SDLCALL SDLNet_SetError(const char *fmt, ...);
extern DECLSPEC const char * SDLCALL SDLNet_GetError(void);

/***********************************************************************/
/* Inline functions to read/write network data                         */
/***********************************************************************/

/* Warning, some systems have data access alignment restrictions */
#if defined(sparc) || defined(mips) || defined(__arm__)
#define SDL_DATA_ALIGNED    1
#endif
#ifndef SDL_DATA_ALIGNED
#define SDL_DATA_ALIGNED    0
#endif

/* Write a 16/32-bit value to network packet buffer */
#define SDLNet_Write16(value, areap) _SDLNet_Write16(value, areap)
#define SDLNet_Write32(value, areap) _SDLNet_Write32(value, areap)

/* Read a 16/32-bit value from network packet buffer */
#define SDLNet_Read16(areap) _SDLNet_Read16(areap)
#define SDLNet_Read32(areap) _SDLNet_Read32(areap)

#if !defined(WITHOUT_SDL) && !SDL_DATA_ALIGNED

SDL_FORCE_INLINE void _SDLNet_Write16(Uint16 value, void *areap)
{
    *(Uint16 *)areap = SDL_SwapBE16(value);
}

SDL_FORCE_INLINE void _SDLNet_Write32(Uint32 value, void *areap)
{
    *(Uint32 *)areap = SDL_SwapBE32(value);
}

SDL_FORCE_INLINE Uint16 _SDLNet_Read16(const void *areap)
{
    return SDL_SwapBE16(*(const Uint16 *)areap);
}

SDL_FORCE_INLINE Uint32 _SDLNet_Read32(const void *areap)
{
    return SDL_SwapBE32(*(const Uint32 *)areap);
}

#else /* !defined(WITHOUT_SDL) && !SDL_DATA_ALIGNED */

SDL_FORCE_INLINE void _SDLNet_Write16(Uint16 value, void *areap)
{
    Uint8 *area = (Uint8*)areap;
    area[0] = (value >>  8) & 0xFF;
    area[1] =  value        & 0xFF;
}

SDL_FORCE_INLINE void _SDLNet_Write32(Uint32 value, void *areap)
{
    Uint8 *area = (Uint8*)areap;
    area[0] = (value >> 24) & 0xFF;
    area[1] = (value >> 16) & 0xFF;
    area[2] = (value >>  8) & 0xFF;
    area[3] =  value        & 0xFF;
}

SDL_FORCE_INLINE Uint16 _SDLNet_Read16(void *areap)
{
    Uint8 *area = (Uint8*)areap;
    return ((Uint16)area[0]) << 8 | ((Uint16)area[1]);
}

SDL_FORCE_INLINE Uint32 _SDLNet_Read32(const void *areap)
{
    const Uint8 *area = (const Uint8*)areap;
    return ((Uint32)area[0]) << 24 | ((Uint32)area[1]) << 16 | ((Uint32)area[2]) << 8 | ((Uint32)area[3]);
}

#endif /* !defined(WITHOUT_SDL) && !SDL_DATA_ALIGNED */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_NET_H */

/*
  SDL_net:  An example cross-platform network library for use with SDL
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* $Id$ */

/* Include normal system headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32_WCE
#include <errno.h>
#endif

/* Include system network headers */
#if defined(__WIN32__) || defined(WIN32)
#define __USE_W32_SOCKETS
#ifdef _WIN64
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <winsock.h>
/* NOTE: windows socklen_t is signed
 * and is defined only for winsock2. */
typedef int socklen_t;
#endif /* W64 */
#include <iphlpapi.h>
#else /* UNIX */
#include <sys/types.h>
#ifdef __FreeBSD__
#include <sys/socket.h>
#endif
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#ifndef __BEOS__
#include <arpa/inet.h>
#endif
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#endif /* WIN32 */

/* FIXME: What platforms need this? */
#if 0
typedef Uint32 socklen_t;
#endif

/* System-dependent definitions */
#ifndef __USE_W32_SOCKETS
#ifdef __OS2__
#define closesocket     soclose
#else  /* !__OS2__ */
#define closesocket close
#endif /* __OS2__ */
#define SOCKET  int
#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1
#endif /* __USE_W32_SOCKETS */

#ifdef __USE_W32_SOCKETS
#define SDLNet_GetLastError WSAGetLastError
#define SDLNet_SetLastError WSASetLastError
#ifndef EINTR
#define EINTR WSAEINTR
#endif
#else
int SDLNet_GetLastError(void);
void SDLNet_SetLastError(int err);
#endif

/*
  SDL_net:  An example cross-platform network library for use with SDL
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>
  Copyright (C) 2012 Simeon Maxein <smaxein@googlemail.com>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* $Id$ */

//#include "SDLnetsys.h"
//#include "SDL_net.h"

#ifdef WITHOUT_SDL
#include <string.h>
#include <stdarg.h>
#endif

const SDLNet_version *SDLNet_Linked_Version(void)
{
    static SDLNet_version linked_version;
    SDL_NET_VERSION(&linked_version);
    return(&linked_version);
}

/* Since the UNIX/Win32/BeOS code is so different from MacOS,
   we'll just have two completely different sections here.
*/
static int SDLNet_started = 0;

#ifndef __USE_W32_SOCKETS
#include <signal.h>
#endif

#ifndef __USE_W32_SOCKETS

int SDLNet_GetLastError(void)
{
    return errno;
}

void SDLNet_SetLastError(int err)
{
    errno = err;
}

#endif

static char errorbuf[1024];

void SDLCALL SDLNet_SetError(const char *fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    SDL_vsnprintf(errorbuf, sizeof(errorbuf), fmt, argp);
    va_end(argp);
#ifndef WITHOUT_SDL
    SDL_SetError("%s", errorbuf);
#endif
}

const char * SDLCALL SDLNet_GetError(void)
{
#ifdef WITHOUT_SDL
    return errorbuf;
#else
    return SDL_GetError();
#endif
}

/* Initialize/Cleanup the network API */
int  SDLNet_Init(void)
{
    if ( !SDLNet_started ) {
#ifdef __USE_W32_SOCKETS
        /* Start up the windows networking */
        WORD version_wanted = MAKEWORD(1,1);
        WSADATA wsaData;

        if ( WSAStartup(version_wanted, &wsaData) != 0 ) {
            SDLNet_SetError("Couldn't initialize Winsock 1.1\n");
            return(-1);
        }
#else
        /* SIGPIPE is generated when a remote socket is closed */
        void (*handler)(int);
        handler = signal(SIGPIPE, SIG_IGN);
        if ( handler != SIG_DFL ) {
            signal(SIGPIPE, handler);
        }
#endif
    }
    ++SDLNet_started;
    return(0);
}
void SDLNet_Quit(void)
{
    if ( SDLNet_started == 0 ) {
        return;
    }
    if ( --SDLNet_started == 0 ) {
#ifdef __USE_W32_SOCKETS
        /* Clean up windows networking */
        if ( WSACleanup() == SOCKET_ERROR ) {
            if ( WSAGetLastError() == WSAEINPROGRESS ) {
#ifndef _WIN32_WCE
                WSACancelBlockingCall();
#endif
                WSACleanup();
            }
        }
#else
        /* Restore the SIGPIPE handler */
        void (*handler)(int);
        handler = signal(SIGPIPE, SIG_DFL);
        if ( handler != SIG_IGN ) {
            signal(SIGPIPE, handler);
        }
#endif
    }
}

/* Resolve a host name and port to an IP address in network form */
int SDLNet_ResolveHost(IPaddress *address, const char *host, Uint16 port)
{
    int retval = 0;

    /* Perform the actual host resolution */
    if ( host == NULL ) {
        address->host = INADDR_ANY;
    } else {
        address->host = inet_addr(host);
        if ( address->host == INADDR_NONE ) {
            struct hostent *hp;

            hp = gethostbyname(host);
            if ( hp ) {
                SDL_memcpy(&address->host,hp->h_addr,hp->h_length);
            } else {
                retval = -1;
            }
        }
    }
    address->port = SDLNet_Read16(&port);

    /* Return the status */
    return(retval);
}

/* Resolve an ip address to a host name in canonical form.
   If the ip couldn't be resolved, this function returns NULL,
   otherwise a pointer to a static buffer containing the hostname
   is returned.  Note that this function is not thread-safe.
*/
/* Written by Miguel Angel Blanch.
 * Main Programmer of Arianne RPG.
 * http://come.to/arianne_rpg
 */
const char *SDLNet_ResolveIP(const IPaddress *ip)
{
    struct hostent *hp;
    struct in_addr in;

    hp = gethostbyaddr((const char *)&ip->host, sizeof(ip->host), AF_INET);
    if ( hp != NULL ) {
        return hp->h_name;
    }

    in.s_addr = ip->host;
    return inet_ntoa(in);
}

int SDLNet_GetLocalAddresses(IPaddress *addresses, int maxcount)
{
    int count = 0;
#ifdef SIOCGIFCONF
/* Defined on Mac OS X */
#ifndef _SIZEOF_ADDR_IFREQ
#define _SIZEOF_ADDR_IFREQ sizeof
#endif
    SOCKET sock;
    struct ifconf conf;
    char data[4096];
    struct ifreq *ifr;
    struct sockaddr_in *sock_addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sock == INVALID_SOCKET ) {
        return 0;
    }

    conf.ifc_len = sizeof(data);
    conf.ifc_buf = (caddr_t) data;
    if ( ioctl(sock, SIOCGIFCONF, &conf) < 0 ) {
        closesocket(sock);
        return 0;
    }

    ifr = (struct ifreq*)data;
    while ((char*)ifr < data+conf.ifc_len) {
        if (ifr->ifr_addr.sa_family == AF_INET) {
            if (count < maxcount) {
                sock_addr = (struct sockaddr_in*)&ifr->ifr_addr;
                addresses[count].host = sock_addr->sin_addr.s_addr;
                addresses[count].port = sock_addr->sin_port;
            }
            ++count;
        }
        ifr = (struct ifreq*)((char*)ifr + _SIZEOF_ADDR_IFREQ(*ifr));
    }
    closesocket(sock);
#elif defined(__WIN32__)
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter;
    PIP_ADDR_STRING pAddress;
    DWORD dwRetVal = 0;
    ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);

    pAdapterInfo = (IP_ADAPTER_INFO *) SDL_malloc(sizeof (IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        return 0;
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == ERROR_BUFFER_OVERFLOW) {
        pAdapterInfo = (IP_ADAPTER_INFO *) SDL_realloc(pAdapterInfo, ulOutBufLen);
        if (pAdapterInfo == NULL) {
            return 0;
        }
        dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
    }

    if (dwRetVal == NO_ERROR) {
        for (pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
            for (pAddress = &pAdapter->IpAddressList; pAddress; pAddress = pAddress->Next) {
                if (count < maxcount) {
                    addresses[count].host = inet_addr(pAddress->IpAddress.String);
                    addresses[count].port = 0;
                }
                ++count;
            }
        }
    }
    SDL_free(pAdapterInfo);
#endif
    return count;
}

#if !defined(WITHOUT_SDL) && !SDL_DATA_ALIGNED /* function versions for binary compatibility */

#undef SDLNet_Write16
#undef SDLNet_Write32
#undef SDLNet_Read16
#undef SDLNet_Read32

/* Write a 16/32 bit value to network packet buffer */
extern DECLSPEC void SDLCALL SDLNet_Write16(Uint16 value, void *area);
extern DECLSPEC void SDLCALL SDLNet_Write32(Uint32 value, void *area);

/* Read a 16/32 bit value from network packet buffer */
extern DECLSPEC Uint16 SDLCALL SDLNet_Read16(void *area);
extern DECLSPEC Uint32 SDLCALL SDLNet_Read32(const void *area);

void  SDLNet_Write16(Uint16 value, void *areap)
{
    (*(Uint16 *)(areap) = SDL_SwapBE16(value));
}

void   SDLNet_Write32(Uint32 value, void *areap)
{
    *(Uint32 *)(areap) = SDL_SwapBE32(value);
}

Uint16 SDLNet_Read16(void *areap)
{
    return (SDL_SwapBE16(*(Uint16 *)(areap)));
}

Uint32 SDLNet_Read32(const void *areap)
{
    return (SDL_SwapBE32(*(Uint32 *)(areap)));
}

#endif /* !defined(WITHOUT_SDL) && !SDL_DATA_ALIGNED */


/*
  SDL_net:  An example cross-platform network library for use with SDL
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* $Id$ */

//#include "SDLnetsys.h"
//#include "SDL_net.h"

/* The network API for TCP sockets */

/* Since the UNIX/Win32/BeOS code is so different from MacOS,
   we'll just have two completely different sections here.
*/

struct _TCPsocket {
    int ready;
    SOCKET channel;
    IPaddress remoteAddress;
    IPaddress localAddress;
    int sflag;
};

/* Open a TCP network socket
   If 'remote' is NULL, this creates a local server socket on the given port,
   otherwise a TCP connection to the remote host and port is attempted.
   The newly created socket is returned, or NULL if there was an error.
*/
TCPsocket SDLNet_TCP_Open(IPaddress *ip)
{
    TCPsocket sock;
    struct sockaddr_in sock_addr;

    /* Allocate a TCP socket structure */
    sock = (TCPsocket)SDL_malloc(sizeof(*sock));
    if ( sock == NULL ) {
        SDLNet_SetError("Out of memory");
        goto error_return;
    }

    /* Open the socket */
    sock->channel = socket(AF_INET, SOCK_STREAM, 0);
    if ( sock->channel == INVALID_SOCKET ) {
        SDLNet_SetError("Couldn't create socket");
        goto error_return;
    }

    /* Connect to remote, or bind locally, as appropriate */
    if ( (ip->host != INADDR_NONE) && (ip->host != INADDR_ANY) ) {

    // #########  Connecting to remote

        SDL_memset(&sock_addr, 0, sizeof(sock_addr));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_addr.s_addr = ip->host;
        sock_addr.sin_port = ip->port;

        /* Connect to the remote host */
        if ( connect(sock->channel, (struct sockaddr *)&sock_addr,
                sizeof(sock_addr)) == SOCKET_ERROR ) {
            SDLNet_SetError("Couldn't connect to remote host");
            goto error_return;
        }
        sock->sflag = 0;
    } else {

    // ##########  Binding locally

        SDL_memset(&sock_addr, 0, sizeof(sock_addr));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_addr.s_addr = INADDR_ANY;
        sock_addr.sin_port = ip->port;

/*
 * Windows gets bad mojo with SO_REUSEADDR:
 * http://www.devolution.com/pipermail/sdl/2005-September/070491.html
 *   --ryan.
 */
#ifndef WIN32
        /* allow local address reuse */
        { int yes = 1;
            setsockopt(sock->channel, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));
        }
#endif

        /* Bind the socket for listening */
        if ( bind(sock->channel, (struct sockaddr *)&sock_addr,
                sizeof(sock_addr)) == SOCKET_ERROR ) {
            SDLNet_SetError("Couldn't bind to local port");
            goto error_return;
        }
        if ( listen(sock->channel, 5) == SOCKET_ERROR ) {
            SDLNet_SetError("Couldn't listen to local port");
            goto error_return;
        }

        /* Set the socket to non-blocking mode for accept() */
#if defined(__BEOS__) && defined(SO_NONBLOCK)
        /* On BeOS r5 there is O_NONBLOCK but it's for files only */
        {
            long b = 1;
            setsockopt(sock->channel, SOL_SOCKET, SO_NONBLOCK, &b, sizeof(b));
        }
#elif defined(O_NONBLOCK)
        {
            fcntl(sock->channel, F_SETFL, O_NONBLOCK);
        }
#elif defined(WIN32)
        {
            unsigned long mode = 1;
            ioctlsocket (sock->channel, FIONBIO, &mode);
        }
#elif defined(__OS2__)
        {
            int dontblock = 1;
            ioctl(sock->channel, FIONBIO, &dontblock);
        }
#else
#warning How do we set non-blocking mode on other operating systems?
#endif
        sock->sflag = 1;
    }
    sock->ready = 0;

#ifdef TCP_NODELAY
    /* Set the nodelay TCP option for real-time games */
    { int yes = 1;
    setsockopt(sock->channel, IPPROTO_TCP, TCP_NODELAY, (char*)&yes, sizeof(yes));
    }
#else
#warning Building without TCP_NODELAY
#endif /* TCP_NODELAY */

    /* Fill in the channel host address */
    sock->remoteAddress.host = sock_addr.sin_addr.s_addr;
    sock->remoteAddress.port = sock_addr.sin_port;

    /* The socket is ready */
    return(sock);

error_return:
    SDLNet_TCP_Close(sock);
    return(NULL);
}

/* Accept an incoming connection on the given server socket.
   The newly created socket is returned, or NULL if there was an error.
*/
TCPsocket SDLNet_TCP_Accept(TCPsocket server)
{
    TCPsocket sock;
    struct sockaddr_in sock_addr;
    socklen_t sock_alen;

    /* Only server sockets can accept */
    if ( ! server->sflag ) {
        SDLNet_SetError("Only server sockets can accept()");
        return(NULL);
    }
    server->ready = 0;

    /* Allocate a TCP socket structure */
    sock = (TCPsocket)SDL_malloc(sizeof(*sock));
    if ( sock == NULL ) {
        SDLNet_SetError("Out of memory");
        goto error_return;
    }

    /* Accept a new TCP connection on a server socket */
    sock_alen = sizeof(sock_addr);
    sock->channel = accept(server->channel, (struct sockaddr *)&sock_addr,
                                &sock_alen);
    if ( sock->channel == INVALID_SOCKET ) {
        SDLNet_SetError("accept() failed");
        goto error_return;
    }
#ifdef WIN32
    {
        /* passing a zero value, socket mode set to block on */
        unsigned long mode = 0;
        ioctlsocket (sock->channel, FIONBIO, &mode);
    }
#elif defined(O_NONBLOCK)
    {
        int flags = fcntl(sock->channel, F_GETFL, 0);
        fcntl(sock->channel, F_SETFL, flags & ~O_NONBLOCK);
    }
#endif /* WIN32 */
    sock->remoteAddress.host = sock_addr.sin_addr.s_addr;
    sock->remoteAddress.port = sock_addr.sin_port;

    sock->sflag = 0;
    sock->ready = 0;

    /* The socket is ready */
    return(sock);

error_return:
    SDLNet_TCP_Close(sock);
    return(NULL);
}

/* Get the IP address of the remote system associated with the socket.
   If the socket is a server socket, this function returns NULL.
*/
IPaddress *SDLNet_TCP_GetPeerAddress(TCPsocket sock)
{
    if ( sock->sflag ) {
        return(NULL);
    }
    return(&sock->remoteAddress);
}

/* Send 'len' bytes of 'data' over the non-server socket 'sock'
   This function returns the actual amount of data sent.  If the return value
   is less than the amount of data sent, then either the remote connection was
   closed, or an unknown socket error occurred.
*/
int SDLNet_TCP_Send(TCPsocket sock, const void *datap, int len)
{
    const Uint8 *data = (const Uint8 *)datap;   /* For pointer arithmetic */
    int sent, left;

    /* Server sockets are for accepting connections only */
    if ( sock->sflag ) {
        SDLNet_SetError("Server sockets cannot send");
        return(-1);
    }

    /* Keep sending data until it's sent or an error occurs */
    left = len;
    sent = 0;
    SDLNet_SetLastError(0);
    do {
        len = send(sock->channel, (const char *) data, left, 0);
        if ( len > 0 ) {
            sent += len;
            left -= len;
            data += len;
        }
    } while ( (left > 0) && ((len > 0) || (SDLNet_GetLastError() == EINTR)) );

    return(sent);
}

/* Receive up to 'maxlen' bytes of data over the non-server socket 'sock',
   and store them in the buffer pointed to by 'data'.
   This function returns the actual amount of data received.  If the return
   value is less than or equal to zero, then either the remote connection was
   closed, or an unknown socket error occurred.
*/
int SDLNet_TCP_Recv(TCPsocket sock, void *data, int maxlen)
{
    int len;

    /* Server sockets are for accepting connections only */
    if ( sock->sflag ) {
        SDLNet_SetError("Server sockets cannot receive");
        return(-1);
    }

    SDLNet_SetLastError(0);
    do {
        len = recv(sock->channel, (char *) data, maxlen, 0);
    } while ( SDLNet_GetLastError() == EINTR );

    sock->ready = 0;
    return(len);
}

/* Close a TCP network socket */
void SDLNet_TCP_Close(TCPsocket sock)
{
    if ( sock != NULL ) {
        if ( sock->channel != INVALID_SOCKET ) {
            closesocket(sock->channel);
        }
        SDL_free(sock);
    }
}


/*
  SDL_net:  An example cross-platform network library for use with SDL
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* $Id$ */

//#include "SDLnetsys.h"
//#include "SDL_net.h"

#ifdef __WIN32__
#define srandom srand
#define random  rand
#endif

struct UDP_channel {
    int numbound;
    IPaddress address[SDLNET_MAX_UDPADDRESSES];
};

struct _UDPsocket {
    int ready;
    SOCKET channel;
    IPaddress address;

    struct UDP_channel binding[SDLNET_MAX_UDPCHANNELS];

    /* For debugging purposes */
    int packetloss;
};

/* Allocate/free a single UDP packet 'size' bytes long.
   The new packet is returned, or NULL if the function ran out of memory.
 */
extern UDPpacket *SDLNet_AllocPacket(int size)
{
    UDPpacket *packet;
    int error;


    error = 1;
    packet = (UDPpacket *)SDL_malloc(sizeof(*packet));
    if ( packet != NULL ) {
        packet->maxlen = size;
        packet->data = (Uint8 *)SDL_malloc(size);
        if ( packet->data != NULL ) {
            error = 0;
        }
    }
    if ( error ) {
        SDLNet_SetError("Out of memory");
        SDLNet_FreePacket(packet);
        packet = NULL;
    }
    return(packet);
}
int SDLNet_ResizePacket(UDPpacket *packet, int newsize)
{
    Uint8 *newdata;

    newdata = (Uint8 *)SDL_malloc(newsize);
    if ( newdata != NULL ) {
        SDL_free(packet->data);
        packet->data = newdata;
        packet->maxlen = newsize;
    }
    return(packet->maxlen);
}
extern void SDLNet_FreePacket(UDPpacket *packet)
{
    if ( packet ) {
        SDL_free(packet->data);
        SDL_free(packet);
    }
}

/* Allocate/Free a UDP packet vector (array of packets) of 'howmany' packets,
   each 'size' bytes long.
   A pointer to the packet array is returned, or NULL if the function ran out
   of memory.
 */
UDPpacket **SDLNet_AllocPacketV(int howmany, int size)
{
    UDPpacket **packetV;

    packetV = (UDPpacket **)SDL_malloc((howmany+1)*sizeof(*packetV));
    if ( packetV != NULL ) {
        int i;
        for ( i=0; i<howmany; ++i ) {
            packetV[i] = SDLNet_AllocPacket(size);
            if ( packetV[i] == NULL ) {
                break;
            }
        }
        packetV[i] = NULL;

        if ( i != howmany ) {
            SDLNet_SetError("Out of memory");
            SDLNet_FreePacketV(packetV);
            packetV = NULL;
        }
    }
    return(packetV);
}
void SDLNet_FreePacketV(UDPpacket **packetV)
{
    if ( packetV ) {
        int i;
        for ( i=0; packetV[i]; ++i ) {
            SDLNet_FreePacket(packetV[i]);
        }
        SDL_free(packetV);
    }
}

/* Since the UNIX/Win32/BeOS code is so different from MacOS,
   we'll just have two completely different sections here.
*/

/* Open a UDP network socket
   If 'port' is non-zero, the UDP socket is bound to a fixed local port.
*/
UDPsocket SDLNet_UDP_Open(Uint16 port)
{
    UDPsocket sock;
    struct sockaddr_in sock_addr;
    socklen_t sock_len;

    /* Allocate a UDP socket structure */
    sock = (UDPsocket)SDL_malloc(sizeof(*sock));
    if ( sock == NULL ) {
        SDLNet_SetError("Out of memory");
        goto error_return;
    }
    SDL_memset(sock, 0, sizeof(*sock));
    SDL_memset(&sock_addr, 0, sizeof(sock_addr));

    /* Open the socket */
    sock->channel = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sock->channel == INVALID_SOCKET )
    {
        SDLNet_SetError("Couldn't create socket");
        goto error_return;
    }

    /* Bind locally, if appropriate */
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = SDLNet_Read16(&port);

    /* Bind the socket for listening */
    if ( bind(sock->channel, (struct sockaddr *)&sock_addr,
            sizeof(sock_addr)) == SOCKET_ERROR ) {
        SDLNet_SetError("Couldn't bind to local port");
        goto error_return;
    }

    /* Get the bound address and port */
    sock_len = sizeof(sock_addr);
    if ( getsockname(sock->channel, (struct sockaddr *)&sock_addr, &sock_len) < 0 ) {
        SDLNet_SetError("Couldn't get socket address");
        goto error_return;
    }

    /* Fill in the channel host address */
    sock->address.host = sock_addr.sin_addr.s_addr;
    sock->address.port = sock_addr.sin_port;

#ifdef SO_BROADCAST
    /* Allow LAN broadcasts with the socket */
    { int yes = 1;
        setsockopt(sock->channel, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
    }
#endif
#ifdef IP_ADD_MEMBERSHIP
    /* Receive LAN multicast packets on 224.0.0.1
       This automatically works on Mac OS X, Linux and BSD, but needs
       this code on Windows.
    */
    /* A good description of multicast can be found here:
        http://www.docs.hp.com/en/B2355-90136/ch05s05.html
    */
    /* FIXME: Add support for joining arbitrary groups to the API */
    {
        struct ip_mreq  g;

        g.imr_multiaddr.s_addr = inet_addr("224.0.0.1");
        g.imr_interface.s_addr = INADDR_ANY;
        setsockopt(sock->channel, IPPROTO_IP, IP_ADD_MEMBERSHIP,
               (char*)&g, sizeof(g));
    }
#endif

    /* The socket is ready */

    return(sock);

error_return:
    SDLNet_UDP_Close(sock);

    return(NULL);
}

void SDLNet_UDP_SetPacketLoss(UDPsocket sock, int percent)
{
    /* FIXME: We may want this behavior to be reproducible
          but there isn't a portable reentrant random
          number generator with good randomness.
    */
    srandom(time(NULL));

    if (percent < 0) {
        percent = 0;
    } else if (percent > 100) {
        percent = 100;
    }
    sock->packetloss = percent;
}

/* Verify that the channel is in the valid range */
static int ValidChannel(int channel)
{
    if ( (channel < 0) || (channel >= SDLNET_MAX_UDPCHANNELS) ) {
        SDLNet_SetError("Invalid channel");
        return(0);
    }
    return(1);
}

/* Bind the address 'address' to the requested channel on the UDP socket.
   If the channel is -1, then the first unbound channel that has not yet
   been bound to the maximum number of addresses will be bound with
   the given address as it's primary address.
   If the channel is already bound, this new address will be added to the
   list of valid source addresses for packets arriving on the channel.
   If the channel is not already bound, then the address becomes the primary
   address, to which all outbound packets on the channel are sent.
   This function returns the channel which was bound, or -1 on error.
*/
int SDLNet_UDP_Bind(UDPsocket sock, int channel, const IPaddress *address)
{
    struct UDP_channel *binding;

    if ( sock == NULL ) {
        SDLNet_SetError("Passed a NULL socket");
        return(-1);
    }

    if ( channel == -1 ) {
        for ( channel=0; channel < SDLNET_MAX_UDPCHANNELS; ++channel ) {
            binding = &sock->binding[channel];
            if ( binding->numbound < SDLNET_MAX_UDPADDRESSES ) {
                break;
            }
        }
    } else {
        if ( ! ValidChannel(channel) ) {
            return(-1);
        }
        binding = &sock->binding[channel];
    }
    if ( binding->numbound == SDLNET_MAX_UDPADDRESSES ) {
        SDLNet_SetError("No room for new addresses");
        return(-1);
    }
    binding->address[binding->numbound++] = *address;
    return(channel);
}

/* Unbind all addresses from the given channel */
void SDLNet_UDP_Unbind(UDPsocket sock, int channel)
{
    if ( (channel >= 0) && (channel < SDLNET_MAX_UDPCHANNELS) ) {
        sock->binding[channel].numbound = 0;
    }
}

/* Get the primary IP address of the remote system associated with the
   socket and channel.
   If the channel is not bound, this function returns NULL.
 */
IPaddress *SDLNet_UDP_GetPeerAddress(UDPsocket sock, int channel)
{
    IPaddress *address;

    address = NULL;
    switch (channel) {
        case -1:
            /* Return the actual address of the socket */
            address = &sock->address;
            break;
        default:
            /* Return the address of the bound channel */
            if ( ValidChannel(channel) &&
                (sock->binding[channel].numbound > 0) ) {
                address = &sock->binding[channel].address[0];
            }
            break;
    }
    return(address);
}

/* Send a vector of packets to the the channels specified within the packet.
   If the channel specified in the packet is -1, the packet will be sent to
   the address in the 'src' member of the packet.
   Each packet will be updated with the status of the packet after it has
   been sent, -1 if the packet send failed.
   This function returns the number of packets sent.
*/
int SDLNet_UDP_SendV(UDPsocket sock, UDPpacket **packets, int npackets)
{
    int numsent, i, j;
    struct UDP_channel *binding;
    int status;
    int sock_len;
    struct sockaddr_in sock_addr;

    if ( sock == NULL ) {
        SDLNet_SetError("Passed a NULL socket");
        return(0);
    }

    /* Set up the variables to send packets */
    sock_len = sizeof(sock_addr);

    numsent = 0;
    for ( i=0; i<npackets; ++i )
    {
        /* Simulate packet loss, if desired */
        if (sock->packetloss) {
            if ((random()%100) <= sock->packetloss) {
                packets[i]->status = packets[i]->len;
                ++numsent;
                continue;
            }
        }

        /* if channel is < 0, then use channel specified in sock */

        if ( packets[i]->channel < 0 )
        {
            sock_addr.sin_addr.s_addr = packets[i]->address.host;
            sock_addr.sin_port = packets[i]->address.port;
            sock_addr.sin_family = AF_INET;
            status = sendto(sock->channel,
                    (const char*)packets[i]->data, packets[i]->len, 0,
                    (struct sockaddr *)&sock_addr,sock_len);
            if ( status >= 0 )
            {
                packets[i]->status = status;
                ++numsent;
            }
        }
        else
        {
            /* Send to each of the bound addresses on the channel */
#ifdef DEBUG_NET
            printf("SDLNet_UDP_SendV sending packet to channel = %d\n", packets[i]->channel );
#endif

            binding = &sock->binding[packets[i]->channel];

            for ( j=binding->numbound-1; j>=0; --j )
            {
                sock_addr.sin_addr.s_addr = binding->address[j].host;
                sock_addr.sin_port = binding->address[j].port;
                sock_addr.sin_family = AF_INET;
                status = sendto(sock->channel,
                        (const char*)packets[i]->data, packets[i]->len, 0,
                        (struct sockaddr *)&sock_addr,sock_len);
                if ( status >= 0 )
                {
                    packets[i]->status = status;
                    ++numsent;
                }
            }
        }
    }

    return(numsent);
}

int SDLNet_UDP_Send(UDPsocket sock, int channel, UDPpacket *packet)
{
    /* This is silly, but... */
    packet->channel = channel;
    return(SDLNet_UDP_SendV(sock, &packet, 1));
}

/* Returns true if a socket is has data available for reading right now */
static int SocketReady(SOCKET sock)
{
    int retval = 0;
    struct timeval tv;
    fd_set mask;

    /* Check the file descriptors for available data */
    do {
        SDLNet_SetLastError(0);

        /* Set up the mask of file descriptors */
        FD_ZERO(&mask);
        FD_SET(sock, &mask);

        /* Set up the timeout */
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        /* Look! */
        retval = select(sock+1, &mask, NULL, NULL, &tv);
    } while ( SDLNet_GetLastError() == EINTR );

    return(retval == 1);
}

/* Receive a vector of pending packets from the UDP socket.
   The returned packets contain the source address and the channel they arrived
   on.  If they did not arrive on a bound channel, the the channel will be set
   to -1.
   This function returns the number of packets read from the network, or -1
   on error.  This function does not block, so can return 0 packets pending.
*/
extern int SDLNet_UDP_RecvV(UDPsocket sock, UDPpacket **packets)
{
    int numrecv, i, j;
    struct UDP_channel *binding;
    socklen_t sock_len;
    struct sockaddr_in sock_addr;

    if ( sock == NULL ) {
        return(0);
    }

    numrecv = 0;
    while ( packets[numrecv] && SocketReady(sock->channel) )
    {
        UDPpacket *packet;

        packet = packets[numrecv];

        sock_len = sizeof(sock_addr);
        packet->status = recvfrom(sock->channel,
                (char*)packet->data, packet->maxlen, 0,
                (struct sockaddr *)&sock_addr,
                        &sock_len);
        if ( packet->status >= 0 ) {
            packet->len = packet->status;
            packet->address.host = sock_addr.sin_addr.s_addr;
            packet->address.port = sock_addr.sin_port;
            packet->channel = -1;

            for (i=(SDLNET_MAX_UDPCHANNELS-1); i>=0; --i )
            {
                binding = &sock->binding[i];

                for ( j=binding->numbound-1; j>=0; --j )
                {
                    if ( (packet->address.host == binding->address[j].host) &&
                         (packet->address.port == binding->address[j].port) )
                    {
                        packet->channel = i;
                        goto foundit; /* break twice */
                    }
                }
            }
foundit:
            ++numrecv;
        }

        else
        {
            packet->len = 0;
        }
    }

    sock->ready = 0;

    return(numrecv);
}

/* Receive a single packet from the UDP socket.
   The returned packet contains the source address and the channel it arrived
   on.  If it did not arrive on a bound channel, the the channel will be set
   to -1.
   This function returns the number of packets read from the network, or -1
   on error.  This function does not block, so can return 0 packets pending.
*/
int SDLNet_UDP_Recv(UDPsocket sock, UDPpacket *packet)
{
    UDPpacket *packets[2];

    /* Receive a packet array of 1 */
    packets[0] = packet;
    packets[1] = NULL;
    return(SDLNet_UDP_RecvV(sock, packets));
}

/* Close a UDP network socket */
extern void SDLNet_UDP_Close(UDPsocket sock)
{
    if ( sock != NULL ) {
        if ( sock->channel != INVALID_SOCKET ) {
            closesocket(sock->channel);
        }
        SDL_free(sock);
    }
}

/*
  SDL_net:  An example cross-platform network library for use with SDL
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* $Id$ */

//#include "SDLnetsys.h"
//#include "SDL_net.h"

/* The select() API for network sockets */

struct SDLNet_Socket {
    int ready;
    SOCKET channel;
};

struct _SDLNet_SocketSet {
    int numsockets;
    int maxsockets;
    struct SDLNet_Socket **sockets;
};

/* Allocate a socket set for use with SDLNet_CheckSockets()
   This returns a socket set for up to 'maxsockets' sockets, or NULL if
   the function ran out of memory.
 */
SDLNet_SocketSet SDLNet_AllocSocketSet(int maxsockets)
{
    struct _SDLNet_SocketSet *set;
    int i;

    set = (struct _SDLNet_SocketSet *)SDL_malloc(sizeof(*set));
    if ( set != NULL ) {
        set->numsockets = 0;
        set->maxsockets = maxsockets;
        set->sockets = (struct SDLNet_Socket **)SDL_malloc
                    (maxsockets*sizeof(*set->sockets));
        if ( set->sockets != NULL ) {
            for ( i=0; i<maxsockets; ++i ) {
                set->sockets[i] = NULL;
            }
        } else {
            SDL_free(set);
            set = NULL;
        }
    }
    return(set);
}

/* Add a socket to a set of sockets to be checked for available data */
int SDLNet_AddSocket(SDLNet_SocketSet set, SDLNet_GenericSocket sock)
{
    if ( sock != NULL ) {
        if ( set->numsockets == set->maxsockets ) {
            SDLNet_SetError("socketset is full");
            return(-1);
        }
        set->sockets[set->numsockets++] = (struct SDLNet_Socket *)sock;
    }
    return(set->numsockets);
}

/* Remove a socket from a set of sockets to be checked for available data */
int SDLNet_DelSocket(SDLNet_SocketSet set, SDLNet_GenericSocket sock)
{
    int i;

    if ( sock != NULL ) {
        for ( i=0; i<set->numsockets; ++i ) {
            if ( set->sockets[i] == (struct SDLNet_Socket *)sock ) {
                break;
            }
        }
        if ( i == set->numsockets ) {
            SDLNet_SetError("socket not found in socketset");
            return(-1);
        }
        --set->numsockets;
        for ( ; i<set->numsockets; ++i ) {
            set->sockets[i] = set->sockets[i+1];
        }
    }
    return(set->numsockets);
}

/* This function checks to see if data is available for reading on the
   given set of sockets.  If 'timeout' is 0, it performs a quick poll,
   otherwise the function returns when either data is available for
   reading, or the timeout in milliseconds has elapsed, which ever occurs
   first.  This function returns the number of sockets ready for reading,
   or -1 if there was an error with the select() system call.
*/
int SDLNet_CheckSockets(SDLNet_SocketSet set, Uint32 timeout)
{
    int i;
    SOCKET maxfd;
    int retval;
    struct timeval tv;
    fd_set mask;

    /* Find the largest file descriptor */
    maxfd = 0;
    for ( i=set->numsockets-1; i>=0; --i ) {
        if ( set->sockets[i]->channel > maxfd ) {
            maxfd = set->sockets[i]->channel;
        }
    }

    /* Check the file descriptors for available data */
    do {
        SDLNet_SetLastError(0);

        /* Set up the mask of file descriptors */
        FD_ZERO(&mask);
        for ( i=set->numsockets-1; i>=0; --i ) {
            FD_SET(set->sockets[i]->channel, &mask);
        }

        /* Set up the timeout */
        tv.tv_sec = timeout/1000;
        tv.tv_usec = (timeout%1000)*1000;

        /* Look! */
        retval = select(maxfd+1, &mask, NULL, NULL, &tv);
    } while ( SDLNet_GetLastError() == EINTR );

    /* Mark all file descriptors ready that have data available */
    if ( retval > 0 ) {
        for ( i=set->numsockets-1; i>=0; --i ) {
            if ( FD_ISSET(set->sockets[i]->channel, &mask) ) {
                set->sockets[i]->ready = 1;
            }
        }
    }
    return(retval);
}

/* Free a set of sockets allocated by SDL_NetAllocSocketSet() */
extern void SDLNet_FreeSocketSet(SDLNet_SocketSet set)
{
    if ( set ) {
        SDL_free(set->sockets);
        SDL_free(set);
    }
}


