#include "parse_cmd.h"
#include "cmd_client.h"
#include "cmd_server.h"
#include "keepalive.h"
#include "console.h"

#define debug_msg(...) consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, __VA_ARGS__)

static uint8_t sendBuff[32];
static uint32_t buffLen;

void parse_client(uint8_t * buff, uint32_t len)
{
	if (buff[0] == CMD_REQEST) {
		switch(buff[1]) {
			case PC_KEEP_ALIVE:
				sendBuff[0] = CMD_ANSWER;
				sendBuff[1] = PC_KEEP_ALIVE;
				cmdClientSend(sendBuff, 2);
				break;
		}
	}
	else if (buff[0] == CMD_ANSWER) {
		switch(buff[1]) {
			case PC_KEEP_ALIVE:
				cmdClientAnswerData(buff, len);
				break;
		}
	}
}

void parse_server(uint8_t * buff, uint32_t len)
{
	if (buff[0] == CMD_REQEST) {
		switch(buff[1]) {
			case PC_KEEP_ALIVE:
				sendBuff[0] = CMD_ANSWER;
				sendBuff[1] = PC_KEEP_ALIVE;
				cmdServerSendData(NULL, sendBuff, 2);
				break;
		}
	}
	else if (buff[0] == CMD_ANSWER) {
		switch(buff[1]) {
			case PC_KEEP_ALIVE:
				cmdServerAnswerData(buff, len);
				break;
		}
	}
}