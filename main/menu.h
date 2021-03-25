#ifndef _MENU_H_
#define _MENU_H_

typedef enum
{
	T_ARG_TYPE_BOOL,
	T_ARG_TYPE_VALUE,
	T_ARG_TYPE_MENU,
	T_ARG_TYPE_START,
	T_ARG_TYPE_WIFI,
	T_ARG_TYPE_INFO,
	T_ARG_TYPE_PARAMETERS
}menu_token_type_t;

typedef struct menu_token
{
	int token;
	char *name;
	char *help;
	void (*new_value)(uint32_t value);
	menu_token_type_t arg_type;
	struct menu_token **menu_list;
	uint32_t * value;
	uint8_t position;
} menu_token_t;

typedef enum
{
	ST_WIFI_FIND_DEVICE,
	ST_WIFI_DEVICE_LIST,
	ST_WIFI_DEVICE_TRY_CONNECT,
	ST_WIFI_DEVICE_CONNECTED_FAILED,
	ST_WIFI_CONNECT,
}stateWifiMenu_t;

extern uint32_t motor_value;
extern uint32_t servo_value;
extern bool motor_on;
extern bool servo_on;

void init_menu(void);
void button_minus_callback(void * arg);
void menuDeactivateButtons(void);
void menuActivateButtons(void);
void menuPrintfInfo(const char *format, ...);
void menuEnterStartFromServer(void);

#endif