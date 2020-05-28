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


void init_menu(void);
void button_minus_callback(void * arg);
void menuDeactivateButtons(void);
void menuActivateButtons(void);
void menuPrintfInfo(const char *format, ...);

#endif