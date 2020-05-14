#include "config.h"
#include "menu.h"

typedef enum
{
	T_ARG_TYPE_BOOL,
	T_ARG_TYPE_VALUE,
	T_ARG_TYPE_MENU,
	T_ARG_TYPE_START,
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
} menu_token_t;

menu_token_t * entered_menu_tab[8];

uint32_t test_val_ = 43;
bool test_bool_;

menu_token_t test_val = 
{
	.name = "test_val",
	.arg_type = T_ARG_TYPE_VALUE,
	.value = &test_val_
};

menu_token_t test_bool = 
{
	.name = "test_bool",
	.arg_type = T_ARG_TYPE_BOOL,
	.value = &test_bool_
};


menu_token_t setings = 
{
	.name = "SETINGS",
	.arg_type = T_ARG_TYPE_MENU,
	.menu_list = 
	{
		&test_val_,
		&test_bool_,
		{0},
	},
};

menu_token_t main_menu = 
{
	.name = "MENU",
	.arg_type = T_ARG_TYPE_MENU,
	.menu_list = 
	{
		&setings,
		{0},
	},
};