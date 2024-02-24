#ifndef _CLIENT_COMMON_H
#define _CLIENT_COMMON_H

#include <WinSock2.h>
#include <stdint.h>

#define STATE_PKT_HEAD 0
#define STATE_PKT_BODY 1


struct pkt_head
{
	uint32_t magic;
	uint32_t size;
	uint32_t flags;
};

struct vec
{
	const void *lpData;
	UINT32 Size;
};


#endif