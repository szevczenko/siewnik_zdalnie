#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/////////////////////  CONFIG PERIPHERALS  ////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
// TINYPRINTF
#define CONFIG_USE_TINYPRINTF TRUE
#define TINYPRINTF_DEFINE_TFP_PRINTF TRUE
#define TINYPRINTF_DEFINE_TFP_SPRINTF TRUE
#define TINYPRINTF_OVERRIDE_LIBC TRUE
///////////////////////////////////////////////////////////////////////////////////////////
// LED
#define CONFIG_USE_LED FALSE
#define CONFIG_LED_STATUS LED1
#define CONFIG_LED_ERROR LED4
///////////////////////////////////////////////////////////////////////////////////////////
// RTT
#define CONFIG_USE_RTT FALSE
#define CONFIG_RTT_CHANNEL 0

///////////////////////////////////////////////////////////////////////////////////////////
// ETHERNET
#define CONFIG_USE_ETH FALSE
#define CONFIG_USE_WIFI TRUE
#define CONFIG_USE_TCPIP TRUE
#define CONFIG_USE_MQTT FALSE
#define CONFIG_DEBUG_LWIP FALSE
///////////////////////////////////////////////////////////////////////////////////////////
// CONSOLE
#define CONFIG_USE_CONSOLE TRUE
#define CONFIG_USE_CONSOLE_TOKEN TRUE
#define CONFIG_USE_CONSOLE_TOKEN_DEBUG FALSE
#define CONFIG_USE_CONSOLE_CMDSHORT FALSE

#define CONFIG_USE_CONSOLE_SERIAL TRUE
#define CONFIG_USE_CONSOLE_TELNET FALSE
#define CONFIG_CONSOLE_ECHO TRUE
#define CONFIG_CONSOLE_PROMPT ""

//////////////////////////////////////  END  //////////////////////////////////////////////

#if (CONFIG_TEST_WDG == TRUE && CONFIG_USE_WDG == FALSE)
#error "Dla testowania WDG aktywuj CONFIG_USE_WDG"
#endif

#if (CONFIG_TEST_CAN == TRUE && (CONFIG_USE_CAN1 == FALSE && CONFIG_USE_CAN2 == FALSE))
#error "Dla testowania CAN aktywuj CONFIG_USE_CAN1 lub CONFIG_USE_CAN2"
#endif

#if (CONFIG_TEST_UNIO == TRUE && CONFIG_USE_UNIO == FALSE)
#error "Dla testowania UNIO aktywuj CONFIG_USE_UNIO"
#endif

#if (CONFIG_USE_CONSOLE == FALSE && CONFIG_USE_SLCAN == TRUE)
#error "SLCAN działa tylko na konsoli"
#endif

//#include "hw.h"

#define NORMALPRIOR 5

#ifndef BOARD_NAME
#define BOARD_NAME                  "ESP-WROOM-32"
#endif

#ifndef BOARD_VERSION
#define BOARD_VERSION {1,0,0}
#endif

#ifndef SOFTWARE_VERSION
#define SOFTWARE_VERSION {1,0,0}
#endif

#ifndef DEFAULT_DEV_TYPE
#define DEFAULT_DEV_TYPE 1
#endif

#if (CONFIG_USE_TCPIP)
#include "lwip/arch.h"
#endif

#if (CONFIG_TEST_WDG == TRUE && CONFIG_USE_WDG == FALSE)
#error "Dla testowania WDG aktywuj CONFIG_USE_WDG"
#endif

#if (CONFIG_TEST_CAN == TRUE && (CONFIG_USE_CAN1 == FALSE && CONFIG_USE_CAN2 == FALSE))
#error "Dla testowania CAN aktywuj CONFIG_USE_CAN1 lub CONFIG_USE_CAN2"
#endif

#if (CONFIG_TEST_UNIO == TRUE && CONFIG_USE_UNIO == FALSE)
#error "Dla testowania UNIO aktywuj CONFIG_USE_UNIO"
#endif

#define NORMALPRIO 12

#define FW_NAME "can-usb"
#define FW_VERSION "v0.1"

typedef struct {
	const uint32_t start_config;
	uint32_t can_id;
	char name[32];
	uint8_t hw_ver[3];
	uint8_t sw_ver[3];
	uint8_t dev_type;
	const uint32_t end_config;
} config_t;

extern config_t config;

int configSave(config_t *config);
int configRead(config_t *config);

enum
{
	_IN = 1,
	_CAN1,
	_CAN2,
	_OUT,
	_DUMP,
	_MP,
	_CONSOLE,
};

typedef enum
{
	ERR_CONFIG_OK = 0,
	ERR_CONFIG_INVALID_CMD = -1,
	ERR_CONFIG_INVALID_ARG = -2,
	ERR_CONFIG_INVALID_NUM = -3,

} err_config_t;

///////////////////////////////////////////////////////////////////////////////////////////
// UART
#define UART_INIT(x) (uart##x##Init)
#define UART_PRINTF(x) (uart##x##Printf)
#define UART_PRINTFI(x) (uart##x##PrintfI)
#define UART_PRINT(x) (uart##x##Print)
#define UART_PRINTI(x) (uart##x##PrintI)
#define UART_DRIVER(x) (UARTD##x)

#define DBG_CAN1 BIT(_CAN1)
#define DBG_CAN2 BIT(_CAN2)

#define DBG_IN BIT(_IN)
#define DBG_OUT BIT(_OUT)
#define DBG_CAN BIT(_CAN)
#define DBG_CAN1 BIT(_CAN1)
#define DBG_CAN2 BIT(_CAN2)
#define DBG_DUMP BIT(_DUMP)
#define DBG_MP BIT(_MP)
#define DBG_CONSOLE BIT(_CONSOLE)

#define debug_printf(format, ...) print(format, ##__VA_ARGS__)
#define CONFIG_BUFF_SIZE 512
#define CONFIG_DEBUG_FLAGS_DEFAULT 0xFFFFFFFF //DBG_DUMP | DBG_OUT
//#define CONFIG_USE_SERIAL_CONSOLE

#define CONFIG_DUMP_FLAGS_DEFAULT 1
//#define CONFIG_DEBUG_FLAGS_DEFAULT 0xFFFFFFFF
#define CONFIG_CON0_PRINTF_FLAGS_DEFAULT 0xFFFFFFFF //DBG_CAN1
#define CONFIG_CON1_PRINTF_FLAGS_DEFAULT 0xFFFFFFFF //DBG_CAN1

#define CONFIG_CON_DEFAULT con0usb
#define CON_PRINTF(format, ...) consolePrintfTimeout(&CONFIG_CON_DEFAULT, TIME_INFINITE, format, ##__VA_ARGS__)
#define cdebug(format, ...) consolePrintfTimeout(&CONFIG_CON_DEFAULT, TIME_INFINITE, format, ##__VA_ARGS__)
#define CONFIG_USE_PANIC FALSE
//#include "assert.h"

// thdTCPShell
#define CONFIG_thdTCPShell_CLIENTS_MAX 1

#define MS2ST(ms) (ms / portTICK_RATE_MS)
#define TIME_IMMEDIATE 0
#define osDelay(ms) vTaskDelay(MS2ST(ms))

//extern portMUX_TYPE osalSysMutex;

#define osalSysLock() portENTER_CRITICAL()
#define osalSysUnlock() portEXIT_CRITICAL()
#define ASSERT() 
#define ESP_OK 0

//#define xTaskCreateStatic(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, none1, none2) xTaskCreate(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, NULL)
typedef int esp_err_t;
///////////////////////////////////////////////////////////////////////////////////////////

void configInit(void);

#endif /* CONFIG_H_ */
