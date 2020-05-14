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

#define NUMBER_CLIENT 5
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
int cmdClientSend(char* buffer, int len);

#endif