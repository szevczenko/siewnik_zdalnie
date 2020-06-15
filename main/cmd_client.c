#include <stdlib.h> // Required for libtelnet.h

#include <lwip/def.h>
#include <lwip/sockets.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include "telnet.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cmd_client.h"

#include "freertos/queue.h"
#include "config.h"
#include "console.h"
#include "keepalive.h"
#include "parse_cmd.h"

#define MAX_VALUE(OLD_V, NEW_VAL) NEW_VAL>OLD_V?NEW_VAL:OLD_V

static uint8_t buffer_cmd[BUFFER_CMD];
uint8_t status_telnet;
static cmd_client_network_t network;
static TaskHandle_t thread_task_handle;

static char cmd_ip_addr[16] = "192.168.4.1";
static uint32_t cmd_port = 8080;

pthread_mutex_t mutex_client;
pthread_mutexattr_t mutexattr;
static keepAlive_t keepAlive;
struct sockaddr_in tcpServerAddr;

#define debug_msg(...) consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, __VA_ARGS__)


int NetworkConnect(char* addr, int port)
{
	int retVal = -1;
    tcpServerAddr.sin_addr.s_addr = inet_addr(addr);
    tcpServerAddr.sin_family = AF_INET;
    tcpServerAddr.sin_port = htons( port );

	if ((network.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		debug_msg( "Failed to allocate socket. errno=%d\n", errno);
		goto exit;
	}

	if ((retVal = connect(network.socket, (struct sockaddr *)&tcpServerAddr, sizeof(tcpServerAddr))) < 0) {
		closesocket(network.socket);
		debug_msg( "socket connect failed errno=%d \n", errno);
		network.socket = -1;
	    goto exit;
	}
exit:
	return retVal;
}


void NetworkDisconnect(void)
{
	close(network.socket);
}


static int read_tcp(unsigned char* buffer, int len, int timeout_ms)
{
	int ret;
	fd_set set;
	FD_ZERO(&set);
	FD_SET(network.socket, &set);
	struct timeval timeout_time;
	timeout_time.tv_sec=timeout_ms/1000;
	timeout_time.tv_usec =(timeout_ms%1000)*1000;
	ret = select(network.socket + 1, &set, NULL, NULL, &timeout_time);
	if (ret < 0) return -1;
	else if (ret == 0) return 0;

	if (FD_ISSET(network.socket , &set)) {
		return read(network.socket, (char *)buffer, len);
	}
	return -1;
}

int cmdClientSend(uint8_t* buffer, uint32_t len)
{
	if (network.socket == -1) {
		return -1;
	}
	int ret = send(network.socket, buffer, len, 0);
	// if (ret >= 0)
	// 	keepAliveAccept(&keepAlive);
	return ret;
}


static void listen_client(void * pv)
{
	int data_len;
	cmdClientStop();
    while(1)
    {
		if (status_telnet == 0)
		{
			if (NetworkConnect(cmd_ip_addr, cmd_port) == -1)
			{
				vTaskDelay(MS2ST(250));
				continue;
			}
			debug_msg("start connect sock = %d\n", network.socket); 
			cmdClientSend((uint8_t*)"Hello World\n",13);
			keepAliveStart(&keepAlive);
			status_telnet = 1;
		}
		data_len = read_tcp(buffer_cmd, sizeof(buffer_cmd), 100);
		if(data_len > 0)
		{
			debug_msg("receive data %d\n", data_len);
			keepAliveAccept(&keepAlive);
			parse_client(buffer_cmd, data_len);
			/* Data Receive */
		}
		else if (data_len < 0)
		{
			debug_msg("client close socket %d\n", data_len);
			cmdClientDisconnect();		
		}
		vTaskDelay(10);
    }// end while
}

static int keepAliveSend(uint8_t * data, uint32_t dataLen) {
	cmdClientSend(data, dataLen);
	//debug_msg("cmdClientSend keepAlive\n");
	return 1;
}

static void cmdClientErrorKACb(void) {
	debug_msg("cmdClientErrorKACb keepAlive\n");
}

void cmdClientStartTask(void)
{
	keepAliveInit(&keepAlive, 800, keepAliveSend, cmdClientErrorKACb);
	xTaskCreate(listen_client, "cmdClientlisten", CONFIG_DO_TELNET_THD_WA_SIZE, NULL, NORMALPRIO, &thread_task_handle);
}

void cmdClientStart(void)
{
	status_telnet = 0;
	vTaskResume(thread_task_handle);
}

void cmdClientSetIp(char * ip)
{
	strcpy(cmd_ip_addr, ip);
}

void cmdClientSetPort(uint32_t port)
{
	cmd_port = port;
}

void cmdClientDisconnect(void)
{
	status_telnet = 0;
	keepAliveStop(&keepAlive);
	close(network.socket);
}

int cmdClientIsConnected(void)
{
	if (status_telnet)
	{
		if (strcmp(inet_ntoa(tcpServerAddr.sin_addr), cmd_ip_addr) == 0 && tcpServerAddr.sin_port == htons(cmd_port))
			return 1;
	}
	return 0;
}

int cmdClientConnect(uint32_t timeout)
{
	if (status_telnet)
	{
		if (cmdClientIsConnected())
			return 1;
		cmdClientDisconnect();
	}
	uint32_t time_now = ST2MS(xTaskGetTickCount());
	do 
	{
		if (status_telnet)
			return 1;
		vTaskDelay(MS2ST(1));
	} while(time_now + timeout < ST2MS(xTaskGetTickCount()));
	return 0;
}

void cmdClientStop(void)
{
	close(network.socket);
	vTaskSuspend(thread_task_handle);
}
