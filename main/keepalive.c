#include "config.h"
#include "freertos/FreeRTOS.h"
#include "keepalive.h"
#include "parse_cmd.h"
#include "console.h"

#define debug_msg(...) consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, __VA_ARGS__)

static keepAlive_t * keepAliveTab[8];
static uint8_t tabSize;

static uint8_t keep_alive_frame[] = {3, CMD_REQEST, PC_KEEP_ALIVE};

void keepAliveInit(keepAlive_t * keep, uint32_t timeout, int (*send)(uint8_t * data, uint32_t dataLen), void (*errorCb)(void)) {
	keep->keepAliveSend = send;
	keep->keepAliveErrorCb = errorCb;
	if (timeout < KEEP_ALIVE_TIME_TO_NEXT)
		keep->timeout = KEEP_ALIVE_TIME_TO_NEXT;
	else
		keep->timeout = timeout;
	keepAliveAccept(keep);
	keep->keepAliveErrorFlag = 0;
	keepAliveTab[tabSize++] = keep;
}

void keepAliveAccept(keepAlive_t * keep)
{
	keep->keepAlive = ST2MS(xTaskGetTickCount()) + keep->timeout;
	keep->keepAliveTry = 0;
	keep->keepAliveErrorFlag = 0;
	//debug_msg("keepalive accept %d timeout %d\n", keep->keepAlive, keep->timeout);
}

int keepAliveCheckError(keepAlive_t * keep) {
	return keep->keepAliveErrorFlag;
}

static void keepAliveProcess(void * pv)
{
	//debug_msg("KeepAliveTask\n");
	keepAlive_t * keep;
	while(1) {

		for(uint8_t i = 0; i < tabSize; i++)
		{
			keep = keepAliveTab[i];

			if (keep == NULL) {
				continue;
			}

			if (keep->keepAliveErrorFlag || keep->keepAliveActiveFlag == 0) {
				continue;
			}
			//debug_msg("keepALive time %d < %d\n", keep->keepAlive, ST2MS(xTaskGetTickCount()));
			if (keep->keepAlive < ST2MS(xTaskGetTickCount())) {
				if (keep->keepAliveTry < KEEP_ALIVE_TRY)
				{
					if (keep->keepAliveSend != NULL) {
						keep->keepAliveSend(keep_alive_frame, sizeof(keep_alive_frame));
					}
					keep->keepAliveTry++;
					keep->keepAlive = ST2MS(xTaskGetTickCount()) + keep->timeout;
				}
				else {
					keep->keepAliveErrorFlag = 1;
					if (keep->keepAliveErrorCb != NULL) {
						keep->keepAliveErrorCb();
					}
				}
			}
		}
		
		vTaskDelay(MS2ST(25));
	}
	
}

void keepAliveStartTask(void)
{
	xTaskCreate(keepAliveProcess, "keepAliveProcess", 1024, NULL, NORMALPRIO, NULL);
}

void keepAliveStart(keepAlive_t * keep)
{
	keep->keepAliveActiveFlag = 1;
	keep->keepAliveErrorFlag = 0;
	keepAliveAccept(keep);
}

void keepAliveStop(keepAlive_t * keep)
{
	keep->keepAliveActiveFlag = 0;
}