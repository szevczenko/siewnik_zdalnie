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

static xSemaphoreHandle waitSemaphore, mutexSemaphore;

#define MAX_VALUE(OLD_V, NEW_VAL) NEW_VAL>OLD_V?NEW_VAL:OLD_V

static uint8_t buffer_cmd[BUFFER_CMD];
uint8_t status_telnet;
static cmd_client_network_t network;
static TaskHandle_t thread_task_handle;
static uint8_t rx_buff[32];
static uint32_t rx_buff_len;

static char cmd_ip_addr[16] = "192.168.4.1";
static uint32_t cmd_port = 8080;

pthread_mutex_t mutex_client;
pthread_mutexattr_t mutexattr;
static keepAlive_t keepAlive;
struct sockaddr_in tcpServerAddr;




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
	return ret;
}

int cmdClientSendDataWaitResp(uint8_t * buff, uint32_t len, uint8_t * buff_rx, uint32_t * rx_len, uint32_t timeout)
{
	if (buff[1] != CMD_REQEST)
		return FALSE;
	if (xSemaphoreTake(mutexSemaphore, timeout) == pdTRUE)
	{
		xQueueReset((QueueHandle_t )waitSemaphore);
		send(network.socket, buff, len, 0);
		if (xSemaphoreTake(waitSemaphore, timeout) == pdTRUE) {
			if (buff[2] != rx_buff[2]) {
				xSemaphoreGive(mutexSemaphore);
				return 0;
			}	
			if (buff_rx != 0)
				memcpy(buff_rx, rx_buff, rx_buff_len);
			if (rx_len != 0)
				*rx_len = rx_buff_len;

			rx_buff_len = 0;
			xSemaphoreGive(mutexSemaphore);
			return 1;
		}
		else {
			xSemaphoreGive(mutexSemaphore);
		}
	}
	debug_msg("Timeout cmdClientSendDataWaitResp\n");
	return FALSE;
}

int cmdClientSetValueWithoutResp(menuValue_t val, uint32_t value) {
	if (menuSetValue(val, value) == FALSE){
		return FALSE;
	}
	uint8_t sendBuff[8];
	sendBuff[0] = 8;
	sendBuff[1] = CMD_DATA;
	sendBuff[2] = PC_SET;
	sendBuff[3] = val;
	memcpy(&sendBuff[4], (uint8_t *)&value, 4);

	return cmdClientSend(sendBuff, 8);
}

int cmdClientSetValueWithoutRespI(menuValue_t val, uint32_t value) {
	int ret_val = TRUE;
	if (menuSetValue(val, value) == FALSE){
		return FALSE;
	}
	uint8_t sendBuff[8];
	sendBuff[0] = 8;
	sendBuff[1] = CMD_DATA;
	sendBuff[2] = PC_SET;
	sendBuff[3] = val;
	memcpy(&sendBuff[4], (uint8_t *)&value, 4);

	taskEXIT_CRITICAL();
	ret_val = cmdClientSend(sendBuff, 8);
	taskENTER_CRITICAL();

	return ret_val;
}

int cmdClientSendCmd(parseCmd_t cmd) {
	int ret_val = TRUE;
	if (cmd > PC_CMD_LAST) { 
		return FALSE;
	}
	uint8_t sendBuff[3];
	sendBuff[0] = 3;
	sendBuff[1] = CMD_COMMAND;
	sendBuff[2] = cmd;

	ret_val = cmdClientSend(sendBuff, 3);

	return ret_val;
}

int cmdClientSendCmdI(parseCmd_t cmd) {
	int ret_val = TRUE;
	if (cmd > PC_CMD_LAST) { 
		return FALSE;
	}
	uint8_t sendBuff[3];
	sendBuff[0] = 3;
	sendBuff[1] = CMD_COMMAND;
	sendBuff[2] = cmd;

	taskEXIT_CRITICAL();
	ret_val = cmdClientSend(sendBuff, 3);
	taskENTER_CRITICAL();

	return ret_val;
}

int cmdClientSetValue(menuValue_t val, uint32_t value, uint32_t timeout_ms) {
	if (menuSetValue(val, value) == 0){
		return 0;
	}
	uint8_t sendBuff[8];
	uint8_t rxBuff[8];
	uint32_t reLen;
	sendBuff[0] = 8;
	sendBuff[1] = CMD_DATA;
	sendBuff[2] = PC_SET;
	sendBuff[3] = val;
	memcpy(&sendBuff[4], (uint8_t *)&value, 4);

	if (cmdClientSendDataWaitResp(sendBuff, 8, rxBuff, &reLen, MS2ST(timeout_ms)) == 0) {
		return -1;
	}
	// debug_msg("cmdClientSetValue receive data %d\n", reLen);
	// for (uint8_t i = 0; i < reLen; i++) {
	// 	debug_msg("%d ", rxBuff[i]);
	// }
	// debug_msg("\n");
	if (rxBuff[2] == POSITIVE_RESP)
		return 1;

	return 0;
}

int cmdClientAnswerData(uint8_t * buff, uint32_t len) {
	if (buff == NULL)
		return FALSE;
	rx_buff_len = len;
	memcpy(rx_buff, buff, rx_buff_len);
	xSemaphoreGive(waitSemaphore);
	return TRUE;
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
			keepAliveStart(&keepAlive);
			status_telnet = 1;
		}
		data_len = read_tcp(buffer_cmd, sizeof(buffer_cmd), 100);
		if(data_len > 0)
		{
			// debug_msg("Client data len %d\n", data_len);
			// for (uint16_t i = 0; i < data_len; i++) {
			// 	debug_msg("%d ", buffer_cmd[i]);
			// }
			// debug_msg("\n\r");
			keepAliveAccept(&keepAlive);
			parse_client_buffer(buffer_cmd, data_len);
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
	return cmdClientSendDataWaitResp(data, dataLen, NULL, NULL, 500);
}

static void cmdClientErrorKACb(void) {
	debug_msg("cmdClientErrorKACb keepAlive\n");
	cmdClientDisconnect();
}

void cmdClientStartTask(void)
{
	keepAliveInit(&keepAlive, 800, keepAliveSend, cmdClientErrorKACb);
	waitSemaphore = xSemaphoreCreateBinary();
	mutexSemaphore = xSemaphoreCreateBinary();
	xSemaphoreGive(mutexSemaphore); 
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
