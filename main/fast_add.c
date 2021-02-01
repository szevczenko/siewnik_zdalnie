#include "config.h"
#include "fast_add.h"


#define FAST_ADD_LIST_SIZE 8
#define CONFIG_FAST_ADD_THD_WA_SIZE 1024

static fast_add_t list[FAST_ADD_LIST_SIZE];
static uint8_t list_cnt;

static SemaphoreHandle_t xSemaphore;

void fastProcessStart(uint32_t * value, uint32_t max, uint32_t min, fast_process_sign sign,  void (*func)(uint32_t))
{
	if (list_cnt >= FAST_ADD_LIST_SIZE) return;
	if( xSemaphoreTake( xSemaphore, ( TickType_t ) 100 ) == pdTRUE )
	{
		for(uint8_t i = 0; i < list_cnt; i++)
		{
			if (value == list[i].value)
			{
				fastProcessStop(value);
				xSemaphoreGive( xSemaphore );
				return;
			}
		}
		list[list_cnt].sign = sign;
		list[list_cnt].max = max;
		list[list_cnt].min = min;
		list[list_cnt].value = value;
		list[list_cnt].counter = 0;
		list[list_cnt].delay = 0;
		list[list_cnt].func = func;
		list_cnt++;
		xSemaphoreGive( xSemaphore );
	}
}

void fastProcessStop(uint32_t * value)
{
	if( xSemaphoreTake( xSemaphore, ( TickType_t ) 100 ) == pdTRUE )
	{
		for(uint8_t i = 0; i < list_cnt; i++)
		{
			if (value == list[i].value)
			{
				memcpy(&list[i], &list[list_cnt - 1], sizeof(fast_add_t));
				memset(&list[list_cnt - 1], 0, sizeof(fast_add_t));
				list_cnt--;
				break;
			}
		}
		xSemaphoreGive( xSemaphore );
	}
}

void fastProcessDeInit(void)
{
	memset(list, 0, sizeof(list));
	list_cnt = 0;
}

static void fast_add(void * arg)
{
	while(1)
	{
		for(uint8_t i = 0; i < list_cnt; i++)
		{
			if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
			{
				if (list[i].counter < 15)
				{
					if (list[i].delay < 1)
					{
						list[i].delay++;
					}
					else
					{
						list[i].delay = 0;
						list[i].counter++;
						if (list[i].sign == FP_PLUS) {
							if ((*list[i].value) < list[i].max) {
								(*list[i].value)++;
							}
						}
						else {
							if ((*list[i].value) > list[i].min) {
								(*list[i].value)--;
							}
						}
						if (list[i].func != NULL)
							list[i].func(*list[i].value);
					}
				}
				else
				{
					list[i].delay = 0;
					list[i].counter++;
					if (list[i].sign == FP_PLUS) {
						if ((*list[i].value) < list[i].max) {
							(*list[i].value)++;
						}
					}
					else {
						if ((*list[i].value) > list[i].min) {
							(*list[i].value)--;
						}
					}
					if (list[i].func != NULL)
						list[i].func(*list[i].value);
				}
				
				xSemaphoreGive( xSemaphore );
			}
		}
		vTaskDelay(MS2ST(35));
	}
	
}

void fastProcessStartTask(void)
{
	xSemaphore = xSemaphoreCreateMutex();
	xSemaphoreGive( xSemaphore );
	xTaskCreate(fast_add, "fast_add", CONFIG_FAST_ADD_THD_WA_SIZE, NULL, NORMALPRIO, NULL);
}
