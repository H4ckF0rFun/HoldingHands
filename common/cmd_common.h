#ifndef CMD_COMMON_H
#define CMD_COMMON_H

#define CMD				('C'|('M')<<8|('D')<<16)

enum CMD_EVENT
{
	CMD_BEGIN,
	CMD_COMMAND,
	CMD_RESULT
};

#endif