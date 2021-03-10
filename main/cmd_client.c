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

#include "keepalive.h"
#include "parse_cmd.h"

static xSemaphoreHandle waitSemaphore, mutexSemaphore;

#define MAX_VALUE(OLD_V, NEW_VAL) NEW_VAL>OLD_V?NEW_VAL:OLD_V

static uint8_t buffer_cmd[BUFFER_CMD];
uint8_t status_telnet;
static cmd_client_network_t network;
static TaskHandle_t thread_task_handle;
static uint8_t rx_buff[128];
static uint32_t rx_buff_len;

static char cmd_ip_addr[16] = "192.168.4.1";
static uint32_t cmd_port = 8080;

pthread_mutex_t mutex_client;
pthread_mutexattr_t mutexattr;
static keepAlive_t keepAlive;
struct sockaddr_in tcpServerAddr;
uint8_t task_status;

int NetworkConnect(char* addr, int port)
{
	debug_function_name("NetworkConnect");
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


static int read_tcp(unsigned char* buffer, int len, int timeout_ms)
{
	debug_function_name("read_tcp");
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
	debug_function_name("cmdClientSend");
	if (network.socket == -1) {
		return -1;
	}
	int ret = send(network.socket, buffer, len, 0);
	return ret;
}

int cmdClientSendDataWaitResp(uint8_t * buff, uint32_t len, uint8_t * buff_rx, uint32_t * rx_len, uint32_t timeout)
{
	debug_function_name("cmdClientSendDataWaitResp");
	if (buff[1] != CMD_REQEST)
		return FALSE;
	//debug_msg("cmdClientSendDataWaitResp start: %d\n\r",len);
	if (xSemaphoreTake(mutexSemaphore, timeout) == pdTRUE)
	{
		xQueueReset((QueueHandle_t )waitSemaphore);
		cmdClientSend(buff, len);
		if (xSemaphoreTake(waitSemaphore, timeout) == pdTRUE) {
			if (buff[2] != rx_buff[2]) {
				xSemaphoreGive(mutexSemaphore);
				memset(rx_buff, 0, sizeof(rx_buff));
				rx_buff_len = 0;
				debug_msg("cmdClientSendDataWaitResp end error: %d\n\r",len);
				return 0;
			}	

			if (rx_len != 0) {
				debug_msg("cmdClientSendDataWaitResp answer len: %d\n\r",rx_buff_len);
				*rx_len = rx_buff_len;
			}

			if (buff_rx != 0) {
				memcpy(buff_rx, rx_buff, rx_buff_len);
			}

			rx_buff_len = 0;
			xSemaphoreGive(mutexSemaphore);
			//debug_msg("cmdClientSendDataWaitResp end: %d\n\r",len);
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
	debug_function_name("cmdClientSetValueWithoutResp");
	if (menuSetValue(val, value) == FALSE){
		return FALSE;
	}
	static uint8_t sendBuff[8];
	sendBuff[0] = 8;
	sendBuff[1] = CMD_DATA;
	sendBuff[2] = PC_SET;
	sendBuff[3] = val;
	memcpy(&sendBuff[4], (uint8_t *)&value, 4);

	return cmdClientSend(sendBuff, 8);
}

int cmdClientSetValueWithoutRespI(menuValue_t val, uint32_t value) {
	debug_function_name("cmdClientSetValueWithoutRespI");
	int ret_val = TRUE;
	if (menuSetValue(val, value) == FALSE){
		return FALSE;
	}
	static uint8_t sendBuff[8];
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

int cmdClientGetValue(menuValue_t val, uint32_t * value, uint32_t timeout) {

	debug_function_name("cmdClientGetValue");
	if (val >= MENU_LAST_VALUE)
		return FALSE;

	if (xSemaphoreTake(mutexSemaphore, timeout) == pdTRUE)
	{
		static uint8_t sendBuff[4];
		sendBuff[0] = 4;
		sendBuff[1] = CMD_REQEST;
		sendBuff[2] = PC_GET;
		sendBuff[3] = val;
		xQueueReset((QueueHandle_t )waitSemaphore);
		cmdClientSend(sendBuff, sizeof(sendBuff));
		if (xSemaphoreTake(waitSemaphore, timeout) == pdTRUE) {
			if (PC_GET != rx_buff[2]) {
				xSemaphoreGive(mutexSemaphore);
				return 0;
			}	

			uint32_t return_value;

			memcpy(&return_value, rx_buff, sizeof(return_value));

			if (menuSetValue(val, return_value) == FALSE) {
				rx_buff_len = 0; 
				xSemaphoreGive(mutexSemaphore);
				return FALSE;
			}

			if (value != 0)
				*value = return_value;

			rx_buff_len = 0;
			xSemaphoreGive(mutexSemaphore);
			return 1;
		}
		else {
			xSemaphoreGive(mutexSemaphore);
			debug_msg("Timeout cmdClientGetValue\n");
		}
	}
	return FALSE;
}

int cmdClientSendCmd(parseCmd_t cmd) {
	debug_function_name("cmdClientSendCmd");
	int ret_val = TRUE;
	if (cmd > PC_CMD_LAST) { 
		return FALSE;
	}
	static uint8_t sendBuff[3];
	sendBuff[0] = 3;
	sendBuff[1] = CMD_COMMAND;
	sendBuff[2] = cmd;

	ret_val = cmdClientSend(sendBuff, 3);

	return ret_val;
}

int cmdClientSendCmdI(parseCmd_t cmd) {
	debug_function_name("cmdClientSendCmdI");
	int ret_val = TRUE;
	if (cmd > PC_CMD_LAST) { 
		return FALSE;
	}
	static uint8_t sendBuff[3];
	sendBuff[0] = 3;
	sendBuff[1] = CMD_COMMAND;
	sendBuff[2] = cmd;

	taskEXIT_CRITICAL();
	ret_val = cmdClientSend(sendBuff, 3);
	taskENTER_CRITICAL();

	return ret_val;
}

int cmdClientSetValue(menuValue_t val, uint32_t value, uint32_t timeout_ms) {
	debug_function_name("cmdClientSetValue");
	if (menuSetValue(val, value) == 0){
		return 0;
	}
	static uint8_t sendBuff[8];
	static uint8_t rxBuff[8];
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

int cmdClientSetAllValue(void) {
	debug_function_name("cmdClientSetAllValue");
	void * data;
	uint32_t data_size;
	static uint8_t sendBuff[128];

	menuParamGetDataNSize(&data, &data_size);

	sendBuff[0] = 3 + data_size;
	sendBuff[1] = CMD_DATA;
	sendBuff[2] = PC_SET_ALL;
	memcpy(&sendBuff[3], data, data_size);

	cmdClientSend(sendBuff, 3 + data_size);

	return TRUE;
}

int cmdClientGetAllValue(uint32_t timeout) {
	debug_function_name("cmdClientGetAllValue");
	if (xSemaphoreTake(mutexSemaphore, timeout) == pdTRUE)
	{
		uint8_t sendBuff[3];

		sendBuff[0] = 3;
		sendBuff[1] = CMD_REQEST;
		sendBuff[2] = PC_GET_ALL;

		xQueueReset((QueueHandle_t )waitSemaphore);
		cmdClientSend(sendBuff, sizeof(sendBuff));
		if (xSemaphoreTake(waitSemaphore, timeout) == pdTRUE) {
			if (PC_GET_ALL != rx_buff[2]) {
				xSemaphoreGive(mutexSemaphore);
				return FALSE;
			}	
			
			uint32_t return_data;
			for (int i = 0; i < (rx_buff_len - 3) / 4; i++) {
				return_data = (uint32_t) rx_buff[3 + i*4];
				//debug_msg("VALUE: %d, %d\n\r", i, return_data);
				if (menuSetValue(i, return_data) == FALSE) {
					debug_msg("Error Set Value %d = %d\n", i, return_data);
				}
			}

			rx_buff_len = 0;
			xSemaphoreGive(mutexSemaphore);
			return TRUE;
		}
		else {
			xSemaphoreGive(mutexSemaphore);
			debug_msg("Timeout cmdServerGetAllValue\n");
		}
	}
	return FALSE;
}

int cmdClientAnswerData(uint8_t * buff, uint32_t len) {
	debug_function_name("cmdClientAnswerData");
	if (buff == NULL)
		return FALSE;
	if (len > sizeof(rx_buff)) {
		debug_msg("cmdClientAnswerData error len %d\n", len);
		return FALSE;
	}
	//debug_msg("cmdClientAnswerData len %d %p\n", len, buff);
	rx_buff_len = len;
	memcpy(rx_buff, buff, rx_buff_len);

	xSemaphoreGiveFromISR(waitSemaphore, NULL);
	return TRUE;
}


static void cmdClientlisten(void * pv)
{
	int data_len;
	cmdClientStop();
    while(1)
    {
		if (task_status == 0) {
			if (network.socket != -1) {
				cmdClientDisconnect();
			}
			vTaskDelay(MS2ST(250));
				continue;
		}
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
		data_len = read_tcp(buffer_cmd, sizeof(buffer_cmd), 5000);
		if(data_len > 0)
		{
			keepAliveAccept(&keepAlive);
			taskENTER_CRITICAL();
			parse_client_buffer(buffer_cmd, data_len);
			taskEXIT_CRITICAL();
			/* Data Receive */
		}
		else if (data_len < 0)
		{
			debug_msg("client close socket %d\n", data_len);
			cmdClientDisconnect();		
		}
    }// end while
}

static int keepAliveSend(uint8_t * data, uint32_t dataLen) {
	return cmdClientSendDataWaitResp(data, dataLen, NULL, NULL, 1000);
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
	network.socket = -1;
	xTaskCreate(cmdClientlisten, "cmdClientlisten", 8192, NULL, NORMALPRIO, &thread_task_handle);
}

void cmdClientStart(void)
{
	if (task_status == 1)
		return;
	status_telnet = 0;
	task_status = 1;
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
	if (status_telnet == 0)
		return;
	status_telnet = 0;
	keepAliveStop(&keepAlive);
	if (network.socket != -1) {
		close(network.socket); 
		network.socket = -1;
	}	
}

int cmdClientIsConnected(void)
{
	return status_telnet;
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
	if (status_telnet == 0)
		return;
	cmdClientDisconnect();
	status_telnet = 0;
}
