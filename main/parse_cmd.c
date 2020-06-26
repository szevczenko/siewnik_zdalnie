#include "parse_cmd.h"
#include "cmd_client.h"
#include "cmd_server.h"
#include "keepalive.h"
#include "console.h"
#include "menu_param.h"

#define debug_msg(...) consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, __VA_ARGS__)

static uint8_t sendBuff[32];
static uint32_t frameLenClient;

void parse_client_buffer(uint8_t * buff, uint32_t len) {
	uint32_t parsed_len = 0;
	do {
		frameLenClient = buff[parsed_len];

		if (frameLenClient > len)
			break;

		parse_client(&buff[parsed_len], frameLenClient);
		len -= frameLenClient;
		parsed_len += frameLenClient;
	}while(len > 0);
}

void parse_client(uint8_t * buff, uint32_t len)
{
	//uint32_t * val;
	uint32_t value;
	if (buff[1] == CMD_REQEST || buff[1] == CMD_DATA) {
		switch(buff[2]) {
			case PC_KEEP_ALIVE:
				sendBuff[0] = 3;
				sendBuff[1] = CMD_ANSWER;
				sendBuff[2] = PC_KEEP_ALIVE;
				if (buff[1] != CMD_DATA)
					cmdClientSend(sendBuff, 3);
				break;

			case PC_GET:
				sendBuff[0] = 8;
				sendBuff[1] = CMD_ANSWER;
				sendBuff[2] = PC_GET;
				sendBuff[3] = POSITIVE_RESP;
				value = menuGetValue(buff[3]);
				memcpy(&sendBuff[4], &value, 4);
				cmdClientSend(sendBuff, 8);
				break;

			case PC_SET:
				sendBuff[0] = 4;
				sendBuff[1] = CMD_ANSWER;
				sendBuff[2] = PC_SET;
				memcpy(&value, &buff[4], 4);
				if (menuSetValue(buff[3], value)) {
					sendBuff[3] = POSITIVE_RESP;
				}
				else {
					sendBuff[3] = NEGATIVE_RESP;
				}
				if (buff[1] != CMD_DATA)
					cmdClientSend(sendBuff, 4);
				break;
		}
	}
	else if (buff[1] == CMD_ANSWER) {
		//debug_msg("Answer data %d", len);
		switch(buff[2]) {
			case PC_SET:
			case PC_GET:
			case PC_KEEP_ALIVE:
				cmdClientAnswerData(buff, len);
				break;
		}
	}
}

static uint32_t frameLenServer;

void parse_server_buffer(uint8_t * buff, uint32_t len) {
	uint32_t parsed_len = 0;
	do {
		frameLenServer = buff[parsed_len];

		if (frameLenClient > len)
			break;

		parse_server(&buff[parsed_len], frameLenServer);
		len -= frameLenServer;
		parsed_len += frameLenServer;
	}while(len > 0);
}

void parse_server(uint8_t * buff, uint32_t len)
{
	//uint32_t * val;
	uint32_t value;
	if (buff[1] == CMD_REQEST || buff[1] == CMD_DATA) {
		switch(buff[2]) {
			case PC_KEEP_ALIVE:
				sendBuff[0] = 3;
				sendBuff[1] = CMD_ANSWER;
				sendBuff[2] = PC_KEEP_ALIVE;
				if (buff[1] != CMD_DATA)
					cmdServerSendData(NULL, sendBuff, 3);
				break;

			case PC_GET:
				sendBuff[0] = 8;
				sendBuff[1] = CMD_ANSWER;
				sendBuff[2] = PC_GET;
				sendBuff[3] = POSITIVE_RESP;
				value = menuGetValue(buff[3]);
				memcpy(&sendBuff[4], &value, 4);
				if (buff[1] != CMD_DATA)
					cmdServerSendData(NULL, sendBuff, 8);
				break;

			case PC_SET:
				sendBuff[0] = 4;
				sendBuff[1] = CMD_ANSWER;
				sendBuff[2] = PC_SET;
				memcpy(&value, &buff[4], 4);
				if (menuSetValue(buff[3], value)) {
					sendBuff[3] = POSITIVE_RESP;
				}
				else {
					sendBuff[3] = NEGATIVE_RESP;
				}
				if (buff[1] != CMD_DATA)
					cmdServerSendData(NULL, sendBuff, 4);
				//menuPrintParameters();
				debug_msg("Sever_positive_set\n\r");
				break;
		}
	}
	else if (buff[1] == CMD_ANSWER) {
		switch(buff[2]) {
			//debug_msg("Answer data %d", len);
			case PC_KEEP_ALIVE:
			case PC_SET:
			case PC_GET:
				cmdServerAnswerData(buff, len);
				break;
		}
	}
}