#ifndef CONSOLE_H_
#define CONSOLE_H_
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "config.h"
#include "tokenline.h"
#include "driver/uart.h"

#if CONFIG_USE_CONSOLE_TELNET
#include "telnet.h"
#endif

#if !defined CONFIG_CONSOLE_PROMPT
#define CONFIG_CONSOLE_PROMPT "> "
#endif

#if !defined CONFIG_CONSOLE_SERIAL
#define CONFIG_CONSOLE_SERIAL UART_NUM_0
#endif

#ifndef CONFIG_CONSOLE_SERIAL_SPEED
#define CONFIG_CONSOLE_SERIAL_SPEED 57600
#endif

#if !defined CONFIG_CONSOLE_TIMEOUT
#define CONFIG_CONSOLE_TIMEOUT 100
#endif

#if !defined CONFIG_CONSOLE_THD_WA_SIZE
#define CONFIG_CONSOLE_THD_WA_SIZE 4096
#endif

#if !defined CONFIG_CONSOLE_THD_CHECK_WA_SIZE
#define CONFIG_CONSOLE_THD_CHECK_WA_SIZE 2048
#endif

#if !defined CONFIG_CONSOLE_THD_PRIORITY
#define CONFIG_CONSOLE_THD_PRIORITY NORMALPRIO + 2
#endif

#if !defined CONFIG_CONSOLE_VSNPRINTF_BUFF_SIZE
#define CONFIG_CONSOLE_VSNPRINTF_BUFF_SIZE 128
#endif

#if CONFIG_USE_CONSOLE

#if CONFIG_USE_CONSOLE_SERIAL && !CONFIG_USE_CONSOLE_USB
#define consolePrint(msg) consolePrintTimeout(&con0serial, msg, CONFIG_CONSOLE_TIMEOUT)
#define consolePrintf(msg, ...) consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, msg,  __VA_ARGS__ )
#endif


typedef enum{
	CON_TYPE_NONE = 0,
	CON_TYPE_USB,
	CON_TYPE_SERIAL,
	CON_TYPE_TELNET,
	CON_TYPE_CAN
}_console_type;

typedef enum {
	CON_MODE_CONSOLE,
	CON_MODE_AT_COM,
}_console_mode;

typedef struct console_t
{
	char *thread_name;
	xTaskHandle *thread_referencep;
	union
	{
#if CONFIG_USE_CONSOLE_SERIAL
		uart_port_t sd;
#endif

#if defined HAL_USE_SERIAL_USB && HAL_USE_SERIAL_USB == TRUE
		SerialUSBDriver *sdu;
#endif

#ifdef HAL_STREAMS_H
		BaseSequentialStream *bss;
#endif

#if CONFIG_USE_CONSOLE_TELNET
		telnet_t ** telnet;
#endif
	};
	t_tokenline *tl;
	int console_type;
	int console_mode;
	uint8_t (*read_method)(struct console_t *, uint8_t *symbol);
	uint8_t timeout;
	xSemaphoreHandle  bsem;
	bool active; //czy task powinien byc aktywny
} console_t;

extern console_t con0usb;
extern console_t con0serial;
extern console_t con0can;

void consoleInit(void);
void consoleCheck(void);
void consoleStart(void);
void consolePrintTimeout(console_t *con, const char *string, uint32_t timeout);
void consolePrintfTimeout(console_t *con, uint32_t timeout, const char *format, ...);
void consolePrint_len(console_t *con, const char *string, uint32_t len);
void consoleThd(void * arg);

#endif //#if CONFIG_USE_CONSOLE

#endif


