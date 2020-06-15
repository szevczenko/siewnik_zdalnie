#include "config.h"
#include "freertos/FreeRTOS.h"
#include "keepalive.h"

static uint32_t keepAlive;
static uint8_t keepAliveTry;
static uint8_t keepAliveErrorFlag;
static TaskHandle_t thread_task_handle;

static int (*keepAliveSend)(uint8_t * data, uint32_t dataLen);
static void (*keepAliveErrorCb)(void);

void keepAliveInit(int (*send)(uint8_t * data, uint32_t dataLen), void (*errorCb)(void)) {
	keepAliveSend = send;
	keepAliveAccept();
	keepAliveErrorFlag = 0;
}

void keepAliveAccept(void)
{
	keepAlive = ST2MS(xTaskGetTickCount()) + KEEP_ALIVE_TIME_TO_NEXT;
	keepAliveTry = 0;
	keepAliveErrorFlag = 0;
}

int keepAliveCheckError(void) {
	return keepAliveErrorFlag;
}

static void keepAliveProcess(void)
{
	while(1) {
		if (keepAliveErrorFlag) {
			vTaskDelay(MS2ST(100));
			continue;
		}

		if (keepAlive < ST2MS(xTaskGetTickCount())) {
			if (keepAliveTry < KEEP_ALIVE_TRY)
			{
				if (keepAliveSend != NULL) {
					(*keepAliveSend)(&keepAliveTry, 1);
				}
				keepAliveTry++;
			}
			else {
				keepAliveErrorFlag = 1;
				if (keepAliveErrorCb != NULL) {
					(*keepAliveErrorCb)();
				}
			}
		}
		vTaskDelay(MS2ST(10));
	}
	
}

void keepAliveStartTask(void)
{
	xTaskCreate(keepAliveProcess, "listen_client", 512, NULL, NORMALPRIO, &thread_task_handle);
}

void keepAliveStart(void)
{
	vTaskResume(thread_task_handle);
	keepAliveAccept();
}

void keepAliveStop(void)
{
	vTaskSuspend(thread_task_handle);
}