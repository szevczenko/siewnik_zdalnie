#ifndef _CMD_CLIENT_H
#define _CMD_CLIENT_H

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/select.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <errno.h>
#include "menu_param.h"

#define PORT     8080 
#define MAXLINE 1024 

#define MSG_OK 1
#define MSG_ERROR 0
#define MSG_TIMEOUT -1

#define BUFFER_CMD 1024

typedef struct 
{
    int socket;
    struct sockaddr_in servaddr; 
}cmd_client_network_t;

void cmdClientStartTask(void);
void cmdClientStart(void);
void cmdClientStop(void);
int cmdClientSend(uint8_t * buffer, uint32_t len);

void cmdClientSetIp(char * ip);
void cmdClientSetPort(uint32_t port);
void cmdClientDisconnect(void);
int cmdClientConnect(uint32_t timeout);
int cmdClientIsConnected(void);
int cmdClientSendDataWaitResp(uint8_t * buff, uint32_t len, uint8_t * buff_rx, uint32_t * rx_len, uint32_t timeout);
int cmdClientAnswerData(uint8_t * buff, uint32_t len);

int cmdClientSetValue(menuValue_t val, uint32_t value, uint32_t timeout_ms);
int cmdClientSetValueWithoutResp(menuValue_t val, uint32_t value);
int cmdClientSetValueWithoutRespI(menuValue_t val, uint32_t value);
int cmdClientGetValue(menuValue_t val, uint32_t * value, uint32_t timeout);
int cmdClientSendCmd(parseCmd_t cmd);
int cmdClientSendCmdI(parseCmd_t cmd);

#endif