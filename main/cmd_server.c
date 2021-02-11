#include <stdlib.h> // Required for libtelnet.h

#include <lwip/def.h>
#include <lwip/sockets.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include "telnet.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cmd_server.h"

#include "freertos/queue.h"
#include "config.h"

#include "parse_cmd.h"
#include "configCmd.h"
#include "menu_param.h"

#define MAX_VALUE(OLD_V, NEW_VAL) NEW_VAL>OLD_V?NEW_VAL:OLD_V

static uint8_t buffer_cmd[BUFFER_CMD];
static uint8_t rx_buff[32];
static uint32_t rx_buff_len;
static uint8_t status_telnet;
static uint8_t error_counter;
static uint8_t starting_flag;
static struct client_network n_clients[NUMBER_CLIENT];
static struct server_network network;
static TaskHandle_t thread_task_handle;
static int max_socket = 0;
static xSemaphoreHandle waitSemaphore, mutexSemaphore;




static void cmdServerError(void) { 
	error_counter++;
	if (error_counter > 30) {
		debug_msg("Error counter overflow. System reset\n");
		configRebootToBlt();
	}
}

static void cmdServerClearError(void) {
	error_counter = 0;
}

static int init_client(void)
{	
    network.socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
    network.servaddr.sin_family = AF_INET;
    network.servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    network.servaddr.sin_port = htons(PORT);
    network.count_clients = 0;
    network.clients = n_clients;
	network.buffer = buffer_cmd;

    int rc = bind(network.socket_tcp, (struct sockaddr *) &network.servaddr,sizeof(network.servaddr));
    if(rc<0)
    {
        debug_msg("bind error %d (%s)\n", errno, strerror(errno));
		close(network.socket_tcp);
		cmdServerError();
        return MSG_ERROR;
    }

    rc = listen(network.socket_tcp, 5);
	if (rc < 0) {
		debug_msg( "listen: %d (%s)\n", errno, strerror(errno));
		close(network.socket_tcp);
		cmdServerError();
		return MSG_ERROR;
	}
    return TRUE;
}

static void deInitClient(void)
{
	status_telnet = 0;
	close(network.socket_tcp);
}


static void listen_client(void * pv)
{
	volatile int rc = -1;
	socklen_t len = sizeof(network.servaddr);
	fd_set set;
	struct timeval time_select;
	int rv;
	uint8_t i;
	 // add our file descriptor to the set
	cmdServerStop();
    while(1)
    {
		if (status_telnet == 0)
		{
			if (init_client() == MSG_ERROR)
			{
				vTaskDelay(MS2ST(250));
				continue;
			}
			max_socket = network.socket_tcp;
			FD_ZERO(&set); // clear the set 
			FD_SET(network.socket_tcp, &set);
			debug_msg("start listen sock = %d\n", network.socket_tcp); 
			status_telnet = 1;
		}
		time_select.tv_sec = 1;
		time_select.tv_usec = 0;

		FD_SET(network.socket_tcp, &set);

		for (i = 0; i<network.count_clients; i++)
		{
			FD_SET(network.clients[i].client_socket, &set);
		} 

        rv = select(max_socket + 1, &set, NULL, NULL, &time_select);  // return the number of file description contained in the three returned sets

	    if (rv<0)
	    {
			deInitClient();
		    debug_msg( "select: %d (%s)\n", errno, strerror(errno)); // an error accured 
	    }
	    else if(rv == 0)
	    {
			int error = 0;
			socklen_t len = sizeof (error);
			int retval = getsockopt (network.socket_tcp, SOL_SOCKET, SO_ERROR, &error, &len);
			if (retval != 0) {
				deInitClient();
			}
	    }
	    else if (FD_ISSET( network.socket_tcp , &set) && rv>0) //Add client
	    {
            if (network.count_clients<NUMBER_CLIENT)
            {
                rc = accept(network.socket_tcp, (struct sockaddr *)&network.servaddr, &len);
		        if (rc < 0) {
					debug_msg( "accept: %d (%s)\n", errno, strerror(errno));
					deInitClient();
					vTaskDelay(MS2ST(100));
					continue;
		        }
				cmdServerClearError();
		        network.clients[network.count_clients].client_socket = rc;
				max_socket = MAX_VALUE(max_socket,rc);
				keepAliveStart(&network.clients[network.count_clients].keepAlive);
                network.count_clients++;
		        debug_msg( "We have a new client connection! %d\n", rc);
            }
            else
            {
                if (max_socket == network.clients[0].client_socket)
				{
					max_socket = network.socket_tcp;
					for (uint8_t j = 0; j<network.count_clients; j++)
					{
						if (j == 0) 
						{
							max_socket = MAX_VALUE(max_socket,network.socket_tcp);
							continue;
						}
						max_socket = MAX_VALUE(max_socket,network.clients[j].client_socket);
					}
				}
				close(network.clients[0].client_socket);
				FD_CLR(network.clients[0].client_socket,&set);
				rc = accept(network.socket_tcp, (struct sockaddr *)&network.servaddr, &len);
		        if (rc < 0) {
					debug_msg( "accept: %d (%s)\n", errno, strerror(errno));
					deInitClient();
					continue;
		        }
				cmdServerClearError();
		        network.clients[0].client_socket = rc;
				max_socket = MAX_VALUE(max_socket,rc);
				vTaskDelay(MS2ST(100));	
            }
		    
	    }
		else if (rv>0)
		{
			for (i = 0; i<network.count_clients;i++) //recieve data from clients
        	{
				if (!FD_ISSET( network.clients[i].client_socket , &set)) continue;
  				int len = read(network.clients[i].client_socket, (char *)network.buffer, BUFFER_CMD);
				if (len == 0 || len < 0)
				{
					if (len < 0)
					{		
						debug_msg("read sock %d i = %d error (%s)\n",network.clients[i].client_socket, i, strerror(errno));
					}
					if (max_socket == network.clients[i].client_socket)
					{
						max_socket = network.socket_tcp;
						for (uint8_t j = 0; j<network.count_clients; j++)
						{
							if (j == i) 
							{
								max_socket = MAX_VALUE(max_socket,network.socket_tcp);
								continue;
							}
							max_socket = MAX_VALUE(max_socket,network.clients[j].client_socket);
						}
					}
					debug_msg( "Client finished %d\n",network.clients[i].client_socket);
					close(network.clients[i].client_socket);
					FD_CLR(network.clients[i].client_socket,&set);
                	if(i != network.count_clients - 1)
                	{
                    	network.clients[i] = network.clients[network.count_clients - 1];
						network.clients[network.count_clients - 1].client_socket = 0;
                	}
                	network.count_clients--;	
					vTaskDelay(MS2ST(100));	
				}
				if (len>0)
				{
					// debug_msg("Server data len %d\n", len);
					// for (uint16_t i = 0; i < len; i++) {
					// 	debug_msg("%d ",network.buffer[i]);
					// }
					// debug_msg("\n\r");
					keepAliveAccept(&network.clients[i].keepAlive);
					parse_server_buffer(network.buffer, len);
				}
        	} //end for
		}//end if
    }// end while
}

void cmdServerSendData(void * arg, uint8_t * buff, uint8_t len)
{
	(void) arg;
	for(uint8_t i = 0; i < network.count_clients; i++)
	{
		send(network.clients[i].client_socket, buff, len, 0);
	}
}

int cmdServerSendDataWaitResp(uint8_t * buff, uint32_t len, uint8_t * buff_rx, uint32_t * rx_len, uint32_t timeout)
{
	if (buff[1] != CMD_REQEST)
		return FALSE;
	
	if (xSemaphoreTake(mutexSemaphore, timeout) == pdTRUE)
	{
		for(uint8_t i = 0; i < network.count_clients; i++) {
			send(network.clients[i].client_socket, buff, len, 0);
		}
	
		if (xSemaphoreTake(waitSemaphore, timeout) == pdTRUE) {

			if (buff[2] != rx_buff[2]) {
				xSemaphoreGive(mutexSemaphore);
				return FALSE;
			}	
			
			if (buff_rx != NULL)
				memcpy(buff_rx, rx_buff, rx_buff_len);
			if (rx_len != NULL)
				*rx_len = rx_buff_len;
			xSemaphoreGive(mutexSemaphore);
			return TRUE;
		}
	}
	
	return FALSE;
}

int cmdServerAnswerData(uint8_t * buff, uint32_t len) {
	if (buff == NULL)
		return FALSE;

	memcpy(rx_buff, buff, rx_buff_len);
	xSemaphoreGive(waitSemaphore);
	return TRUE;
}

int cmdServerSetValueWithoutResp(menuValue_t val, uint32_t value) {
	if (menuSetValue(val, value) == FALSE){
		return FALSE;
	}
	static uint8_t sendBuff[8];
	sendBuff[0] = 8;
	sendBuff[1] = CMD_DATA;
	sendBuff[2] = PC_SET;
	sendBuff[3] = val;
	memcpy(&sendBuff[4], (uint8_t *)&value, 4);

	cmdServerSendData(NULL, sendBuff, 8);
	return TRUE;
}

int cmdServerSetValueWithoutRespI(menuValue_t val, uint32_t value) {
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
	cmdServerSendData(NULL, sendBuff, 8);
	taskENTER_CRITICAL();

	return TRUE;
}

static int keepAliveSend(uint8_t * data, uint32_t dataLen) {
	return cmdServerSendDataWaitResp(data, dataLen, NULL, NULL, 500);
}

static void cmdServerErrorKACb(void) {
	debug_msg("cmdServerErrorKACb keepAlive");
	for (uint8_t j = 0; j<network.count_clients; j++)
	{
		close(network.clients[j].client_socket);
	}
	max_socket = network.socket_tcp; 
}

void cmdServerStartTask(void)
{
	for (uint8_t i = 0; i < NUMBER_CLIENT; i++) {
		keepAliveInit(&n_clients[i].keepAlive, 1000, keepAliveSend, cmdServerErrorKACb);
	}
	waitSemaphore = xSemaphoreCreateBinary();
	mutexSemaphore = xSemaphoreCreateBinary();
	xSemaphoreGive(mutexSemaphore); 
	xTaskCreate(listen_client, "listen_client", CONFIG_DO_TELNET_THD_WA_SIZE, NULL, NORMALPRIO, &thread_task_handle);
}

void cmdServerStart(void)
{
	deInitClient();
	cmdServerClearError();
	vTaskResume(thread_task_handle);
}

void cmdServerStop(void)
{
	close(network.socket_tcp);
	cmdServerClearError();
	for (uint8_t i = 0; i<network.count_clients;i++) //recieve data from clients
    {
		keepAliveStop(&n_clients[i].keepAlive);
		close(network.clients[i].client_socket);
	}
	network.count_clients = 0;
	vTaskSuspend(thread_task_handle);
}
