#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"

#define BUZZER_ON()
#define BUZZER_OFF()
#define BUZZER_INIT()

static uint32_t buzzer_timer;

void buzzer_click(void)
{
	buzzer_timer = xTaskGetTickCount() + MS2ST(100);
	BUZZER_ON();
	debug_msg("buzzer on \n\r");
}

static void buzzer_task(void)
{
	
	while(1)
	{
		if (buzzer_timer != 0 && buzzer_timer < xTaskGetTickCount())
		{
			BUZZER_OFF();
			buzzer_timer = 0;
			debug_msg("buzzer off\n\r");
		}
		vTaskDelay(MS2ST(20));
	}
}

void buzzer_init(void)
{
	BUZZER_INIT();
	BUZZER_OFF();
	xTaskCreate(buzzer_task, "buzzer_task", 1024, NULL, 12, NULL);
}