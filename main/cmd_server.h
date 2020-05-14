#ifndef _CMD_SERVER_H
#define _CMD_SERVER_H

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

struct client_network
{
    int client_socket;
    struct sockaddr_in cliaddr;
};

struct circular_buff
{
    uint8_t * buffer;
    uint8_t flag;
    uint16_t * LenToDo;
    uint16_t pos;
    uint16_t buff_len;
};

struct server_network
{
    int socket_tcp;
    int count_clients;
    uint8_t * buffer;
    struct sockaddr_in servaddr;
    struct sockaddr_in servaddr_udp;  
    struct client_network * clients;  
};

extern struct server_network network;

void cmdServerStartTask(void);
void cmdServerStart(void);
void cmdServerStop(void);

#endif