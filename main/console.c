#include <stdio.h>
#include <stdarg.h>
#include "console.h"
#include "freertos/FreeRTOS.h"
#include "token.h"
#include "cmd_utils.h"
#include "config.h"
#include "atmega_communication.h"

#include "driver/gpio.h"

#if CONFIG_USE_CONSOLE //&& !CONSOLE_USE_ETH

#if CONFIG_USE_CONSOLE_SERIAL
t_tokenline tl_con_serial;
#endif


#if CONFIG_USE_CONSOLE_SERIAL
TaskHandle_t serialTaskHandle;
#endif

void consoleCheckThd(void * pv);
uint8_t consoleGetCharTimeout(console_t *con, uint8_t *symbol);

#if CONFIG_USE_CONSOLE_SERIAL
console_t con0serial =
	{
		.thread_referencep = &serialTaskHandle,
		.thread_name = "console serial", 
		.sd = CONFIG_CONSOLE_SERIAL, 
		.tl = &tl_con_serial,
		.console_type = CON_TYPE_SERIAL, 
		.read_method = consoleGetCharTimeout
	};
#endif //CONFIG_USE_CONSOLE_SERIAL

uint8_t consoleGetCharTimeout(console_t *con, uint8_t *symbol)
{
	uint32_t timeout = TIME_IMMEDIATE / portTICK_RATE_MS;
	if (con->console_type == CON_TYPE_SERIAL)
	{
#if CONFIG_USE_CONSOLE_SERIAL
		if (uart_read_bytes(con->sd, (uint8_t *)symbol, 1, timeout) <= 0)
			return 0;
#endif
	}
	return 1;
}

#if CONFIG_USE_CONSOLE_TOKEN
void execute(void *user, t_tokenline_parsed *p)
{
	console_t *con;
	int i;

	con = user;

	if (debug_flags & DEBUG_TOKENLINE)
		debug_token_dump(con, p);

	else
	{
		for (i = 0; tokenMap[i].token; i++)
		{
			if (p->tokens[0] == tokenMap[i].token)
			{
				if (!tokenMap[i].func(con, p))
					cprint(con, "ERR: Command failed.\r\n");
				break;
			}
		}
		if (!tokenMap[i].token)
		{
			cprint(con, "ERR: Command mapping not found.\r\n");
		}
	}
}
#endif // CONFIG_USE_CONSOLE_TOKEN

void consolePrintTimeout(console_t *con, const char *string, uint32_t timeout)
{
	(void) timeout;
	cprint(con, string);
}

void consolePrint_len(console_t *con, const char *string, uint32_t len)
{
	if (con == NULL)
	{
		cprint_len(con, string, len);
		return;
	}
	if (con->active == true)
	{
		//if (xSemaphoreTake((con->bsem), 100) == pdTRUE)
		{
			cprint_len(con, string, len);
			xSemaphoreGive((con->bsem));
		}
	}
}

void consolePrintfTimeout(console_t *con, uint32_t timeout, const char *format, ...)
{
	static char buff[CONFIG_CONSOLE_VSNPRINTF_BUFF_SIZE];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buff, CONFIG_CONSOLE_VSNPRINTF_BUFF_SIZE, format, ap);

	if (con == NULL)
	{
		cprint(con, buff);
		return;
	}
	//if (xSemaphoreTake((con->bsem), timeout) == pdTRUE)
	{
		cprint(con, buff);
		xSemaphoreGive((con->bsem));
	}
}

void consoleThd(void * arg)
{
	console_t *con = arg;
	char input, error;
	vTaskSuspend(*((con)->thread_referencep));

#if CONFIG_USE_CONSOLE_TOKEN
	tl_init(con->tl, tl_tokens, tl_dict, cprint, con);
#else
	tl_init(con->tl, NULL, NULL, cprint, con);
#endif //CONFIG_USE_CONSOLE_TOKEN

	tl_set_prompt(con->tl, CONFIG_CONSOLE_PROMPT);

#if CONFIG_USE_CONSOLE_TOKEN
	tl_set_callback(con->tl, execute);
#endif //CONFIG_USE_CONSOLE_TOKEN
	
	while (1)
	{
		error = con->read_method((console_t *)con, (uint8_t *)&input);
		if (error > 0)
		{
			switch(con->console_mode) {
				case CON_MODE_CONSOLE:
					tl_input(con->tl, input);
				break;

				case CON_MODE_AT_COM:
					at_read_byte((uint8_t) input);
				break;

				default:
					tl_input(con->tl, input);
				break;
			}
		}
		else
		{
			osDelay(20);
		}
		

		if (con->active == false)
		{
			osalSysLock();
			vTaskSuspend(*((con)->thread_referencep));
			osalSysUnlock();
		}
	}
}

static uint8_t serial_init_flag;
static void uart_init(uint32_t baud_rate) {
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    // We won't use a buffer for sending data.
    if (uart_driver_install(CONFIG_CONSOLE_SERIAL, 256, 256, 0, NULL, 0) != ESP_OK) {
        printf("UART: uart_driver_install error\n\r");
        return;
    }
    if (uart_param_config(CONFIG_CONSOLE_SERIAL, &uart_config) != ESP_OK) {
        printf("UART: uart_param_config error\n\r");
        return;
    }
	serial_init_flag = 1;
}

void consoleInit(void)
{
	#if CONFIG_USE_CONSOLE_SERIAL
	con0serial.bsem = xSemaphoreCreateBinary();
	uart_init(CONFIG_CONSOLE_SERIAL_SPEED);	
	#endif
}


void consoleCheck(void)
{
#if CONFIG_USE_CONSOLE_SERIAL
	if (*(con0serial.thread_referencep) != NULL) { 
		if (serial_init_flag && eTaskGetState(*(con0serial.thread_referencep)) == eSuspended)
		{
			vTaskResume(*(con0serial.thread_referencep));
			con0serial.active = true;
		}
		else if (serial_init_flag == 0 && eTaskGetState(*(con0serial.thread_referencep)) != eSuspended)
			con0serial.active = false;
	}
#endif
}

void consoleCheckThd(void * pv)
{
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	while(1)
	{
		consoleCheck();
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}
	
}

void consoleStart(void)
{
	consoleInit();
	xTaskCreate(consoleCheckThd, "consoleCheck", CONFIG_CONSOLE_THD_CHECK_WA_SIZE, NULL, 5, NULL);
	xTaskCreate(consoleThd, con0serial.thread_name, CONFIG_CONSOLE_THD_WA_SIZE, &con0serial, CONFIG_CONSOLE_THD_PRIORITY, con0serial.thread_referencep);
}

#endif //#if CONFIG_USE_CONSOLE