#ifndef CHAT_COMMON_H
#define CHAT_COMMON_H


#define CHAT		('C'|('H'<<8)|('A'<<16)|('T'<<24))


enum  CHAR_EVENT
{
	CHAT_INIT,
	CHAT_BEGIN,
	CHAT_MSG
};

#endif

