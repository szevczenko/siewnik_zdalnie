#ifndef _TOKEN_H_
#define _TOKEN_H_

#include "console.h"
#include "tokenline.h"

#if CONFIG_USE_CONSOLE_TOKEN

enum
{
	T_HELP = 1,
	T_CLEAR,
#if CONFIG_USE_CONSOLE_TOKEN_DEBUG
	T_DEBUG,
#endif //CONFIG_USE_CONSOLE_TOKEN_DEBUG
	T_CONFIG,
#if CONFIG_USE_SERIAL_PLOT
	T_MONITOR,
#endif
	T_ECHO,
	T_MQTT,
	T_ADDRESS,
	T_PORT,
	
	T_ON,
	T_OFF,
	T_LIST,
	T_SET,
	T_ADD,
	T_RESET,
	T_SAVE,
	T_BOOT,
	T_PERIOD,
	
	T_CAN_ID,
	T_DEV_TYPE,

	T_DEV_TYPE_SERVER,
	T_DEV_TYPE_CLIENT,

	#if CONFIG_USE_SERIAL_PLOT
	T_TEST_CH_1,
	T_TEST_CH_2,
	T_TEST_CH_3,
	T_MONITOR_START_SEQ,
	#endif

	T_WIFI,
	T_SSID,
	T_SSID_NUMBER,
	T_PASSWORD,
	T_CONNECT,
	T_SHOW,

	T_TOKENLINE,
};

typedef int (*cmdfunc)(console_t *con, t_tokenline_parsed *p);

typedef struct
{
	int token;
	cmdfunc func;
} tokenMap_t;

extern tokenMap_t tokenMap[];
extern t_token tl_tokens[];
extern t_token_dict tl_dict[];

#endif //#if CONFIG_USE_CONSOLE_TOKEN

#endif
