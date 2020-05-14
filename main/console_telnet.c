#include <stdio.h>
#include <stdarg.h>
#include "console.h"
#include "freertos/FreeRTOS.h"
#include "token.h"
#include "cmd_utils.h"
#include "config.h"
#include "wifidrv.h"
#if CONFIG_USE_CONSOLE_TELNET
#include "libtelnet.h"
#include "telnet.h"

#define CONFIG_ETHCONSOLE_THD_WA_SIZE (4096)

static t_tokenline tl_con_telnet[TELNET_MAX_CLIENT];
static TaskHandle_t telnetTaskHandle[TELNET_MAX_CLIENT];

static char threadName[TELNET_MAX_CLIENT][20];

static uint8_t consoleGetCharTimeout(console_t *con, uint8_t *symbol);

console_t con0telnet[TELNET_MAX_CLIENT];

void consoleEthInit(void)
{
	for (uint8_t i = 0; i<TELNET_MAX_CLIENT; i++)
	{
		con0telnet[i].telnet = &tnHandle[i];
		con0telnet[i].tl = &tl_con_telnet[i];
		con0telnet[i].read_method = consoleGetCharTimeout;
		con0telnet[i].active = false;
		con0telnet[i].thread_referencep = &telnetTaskHandle[i];
		con0telnet[i].bsem = xSemaphoreCreateBinary();
		con0telnet[i].console_type = CON_TYPE_TELNET;
		sprintf(threadName[i], "console_telnet_%d", i);
		con0telnet[i].thread_name = threadName[i];
		xSemaphoreGive((con0telnet[i].bsem));
	}
}

static uint8_t consoleGetCharTimeout(console_t *con, uint8_t *symbol)
{
	uint32_t timeout = TIME_IMMEDIATE;
	(void) timeout;
	struct telnetUserData *telnetData = (((telnet_w_t *)*con->telnet)->ud);
	if (telnetData == NULL)
		return 0;
	if (telnetData->read_char(telnetData, (char *)symbol) == 0)
		return 0;
	return 1;
}

void setStatusEthConsole(uint8_t console, uint8_t status)
{
	con0telnet[console].active = status;
	if (status == 1)
	{
		vTaskResume(*(con0telnet[console].thread_referencep));
		if (cosole_echo_flag == 1)
			consolePrintfTimeout(&con0telnet[console], 500, "Telnet user %d\n\r", console);
	}
}

void consoleTelentStart(void)
{
	for (uint8_t i = 0; i<TELNET_MAX_CLIENT; i++)
	{
		xTaskCreate(consoleThd, con0telnet[i].thread_name, CONFIG_ETHCONSOLE_THD_WA_SIZE, &con0telnet[i], CONFIG_CONSOLE_THD_PRIORITY, con0telnet[i].thread_referencep);
	}
}

#endif