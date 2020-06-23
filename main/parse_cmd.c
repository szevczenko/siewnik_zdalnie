#include "parse_cmd.h"
#include "cmd_client.h"
#include "cmd_server.h"
#include "keepalive.h"
#include "console.h"
#include "menu_param.h"

#define debug_msg(...) consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, __VA_ARGS__)

static uint8_t sendBuff[32];
static uint32_t buffLen;

void parse_client(uint8_t * buff, uint32_t len)
{
	uint32_t * val;
	if (buff[0] == CMD_REQEST) {
		switch(buff[1]) {
			case PC_KEEP_ALIVE:
				sendBuff[0] = CMD_ANSWER;
				sendBuff[1] = PC_KEEP_ALIVE;
				cmdClientSend(sendBuff, 2);
				break;

			case PC_GET:
				sendBuff[0] = CMD_ANSWER;
				sendBuff[1] = PC_GET;
				sendBuff[2] = POSITIVE_RESP;
				val = (uint32_t *)&sendBuff[3];
				*val = menuGetValue(buff[2]);
				cmdClientSend(sendBuff, 7);
				break;

			case PC_SET:
				sendBuff[0] = CMD_ANSWER;
				sendBuff[1] = PC_SET;
				val = (uint32_t *)&buff[3];
				if (menuSetValue(buff[2], *val)) {
					sendBuff[2] = POSITIVE_RESP;
				}
				else {
					sendBuff[2] = NEGATIVE_RESP;
				}
				cmdClientSend(sendBuff, 3);
				break;
		}
	}
	else if (buff[0] == CMD_ANSWER) {
		//debug_msg("Answer data %d", len);
		switch(buff[1]) {
			case PC_SET:
			case PC_GET:
			case PC_KEEP_ALIVE:
				cmdClientAnswerData(buff, len);
				break;
		}
	}
}

void parse_server(uint8_t * buff, uint32_t len)
{
	uint32_t * val;
	uint32_t value;
	if (buff[0] == CMD_REQEST) {
		switch(buff[1]) {
			case PC_KEEP_ALIVE:
				sendBuff[0] = CMD_ANSWER;
				sendBuff[1] = PC_KEEP_ALIVE;
				cmdServerSendData(NULL, sendBuff, 2);
				break;

			case PC_GET:
				sendBuff[0] = CMD_ANSWER;
				sendBuff[1] = PC_GET;
				sendBuff[2] = POSITIVE_RESP;
				val = (uint32_t *)&sendBuff[3];
				*val = menuGetValue(buff[2]);
				cmdServerSendData(NULL, sendBuff, 7);
				break;

			case PC_SET:
				sendBuff[0] = CMD_ANSWER;
				sendBuff[1] = PC_SET;
				memcpy(&value, &buff[3], 4);
				//val = (uint32_t *)&buff[3];
				//debug_msg("Receive set data %d %d\n", len, value);
				if (menuSetValue(buff[2], value)) {
					sendBuff[2] = POSITIVE_RESP;
				}
				else {
					sendBuff[2] = NEGATIVE_RESP;
				}
				cmdServerSendData(NULL, sendBuff, 3);
				//menuPrintParameters();
				break;
		}
	}
	else if (buff[0] == CMD_ANSWER) {
		switch(buff[1]) {
			//debug_msg("Answer data %d", len);
			case PC_KEEP_ALIVE:
			case PC_SET:
			case PC_GET:
				cmdServerAnswerData(buff, len);
				break;
		}
	}
}