#ifndef _PARSE_CMD_H
#define _PARSE_CMD_H
#include "config.h"
#include "communication.h"

#define CMD_REQEST 0x11
#define CMD_ANSWER 0x22
#define CMD_DATA 0x33
typedef enum
{
	PC_KEEP_ALIVE,
	PC_LAST
}parseCmd_t;

void parse_client(uint8_t * buff, uint32_t len);
void parse_server(uint8_t * buff, uint32_t len);

#endif