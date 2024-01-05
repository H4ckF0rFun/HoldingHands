#ifndef _SOCKS_COMMON_H
#define _SOCKS_COMMON_H




#define MAX_CLIENT_COUNT	4096

#define UDP_MAX_BUFF		0x10000
#define TCP_MAX_BUFF		0x10000


//socks4 state...
#define SOCKS4_VER		0
#define SOCKS4_CMD		1
#define SOCKS4_PORT		2
#define SOCKS4_ADDR		3
#define SOCKS4_USERID	4

//socks 5 state
#define SOCKS5_VER			0
#define SOCKS5_METHOD_LEN	1
#define SOCKS5_AUTH_METHOD	2

//Request :
//	ver,cmd,\x00,atyp,				port...

#define SOCKS5_CMD			1
#define SOCKS5_RSV			2
#define SOCKS5_ATYP			3

//cmd

#define SOCKS_CMD_CONNECT		0x1
#define SOCKS_CMD_UDPASSOCIATE	0x3


#define ADDRTYPE_IPV4			0x1
#define ADDRTYPE_DOMAIN			0x3
#define ADDRTYPE_IPV6			0x4

//
#define SOCKS_PROXY			(('S') | (('K') << 8) | (('P') << 16) | (('X') << 24))

enum SOCKS_PROXY_EVENT
{
	SOCK_PROXY_REQUEST,
	SOCK_PROXY_REQUEST_RESULT,
	SOCK_PROXY_DATA,
	SOCK_PROXY_CLOSE
};

#endif