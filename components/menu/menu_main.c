#include "config.h"
#include "menu.h"
#include "menu_drv.h"
#include "ssd1306.h"
#include "ssdFigure.h"

#define LINE_HEIGHT 10
#define MENU_HEIGHT 18
#define MAX_LINE (SSD1306_HEIGHT - MENU_HEIGHT) / LINE_HEIGHT
#define NULL_ERROR_MSG() debug_msg("Error menu pointer list is NULL (%s)\n\r", __func__);

static scrollBar_t scrollBar = {
	.line_max = MAX_LINE,
	.y_start = MENU_HEIGHT
};

menu_token_t setings = 
{
	.name = "SETINGS",
	.arg_type = T_ARG_TYPE_MENU,
	//.menu_list = setting_tokens
};

menu_token_t wifi_menu = 
{
	.name = "DEVICES",
};

menu_token_t* main_menu_tokens[] = {&setings, &wifi_menu, NULL};

menu_token_t main_menu = 
{
	.name = "MENU",
	.arg_type = T_ARG_TYPE_MENU,
	.menu_list = main_menu_tokens
};

void menu_button_up_callback(void * arg)
{
	menu_token_t *menu = arg;
	if (menu == NULL)
	{
		NULL_ERROR_MSG()
		return;
	}

	menu->last_button = LAST_BUTTON_UP;
	if (menu->position > 0) 
	{
		menu->position--;
	}
}

void menu_button_down_callback(void * arg)
{
	menu_token_t *menu = arg;
	if (menu == NULL)
	{
		NULL_ERROR_MSG()
		return;
	}
	
	menu->last_button = LAST_BUTTON_DOWN;
	if (menu->position < menuDrvElementsCnt(menu) - 1) 
	{
		menu->position++;
	}
}

void menu_button_enter_callback(void * arg)
{
	menu_token_t *menu = arg;
	if (menu->menu_list[menu->position] == NULL)
	{
		NULL_ERROR_MSG()
		return;
	}
	menuEnter(menu->menu_list[menu->position]);
}

void menu_button_exit_callback(void * arg)
{
	menu_token_t *menu = arg;
	if (menu == NULL)
	{
		NULL_ERROR_MSG()
		return;
	}
	menuExit(menu);
}

bool menu_button_init_cb(void * arg)
{
	menu_token_t *menu = arg;
	if (menu == NULL)
	{
		NULL_ERROR_MSG()
		return false;
	}
	menu->button.down.fall_callback = menu_button_down_callback;
	menu->button.up.fall_callback = menu_button_up_callback;
	menu->button.enter.fall_callback = menu_button_enter_callback;
	menu->button.exit.fall_callback = menu_button_exit_callback;
	return true;
}

bool menu_enter_cb(void * arg)
{
	menu_token_t *menu = arg;
	if (menu == NULL)
	{
		NULL_ERROR_MSG()
		return false;
	}
	return true;
}

bool menu_exit_cb(void * arg)
{
	menu_token_t *menu = arg;
	if (menu == NULL)
	{
		NULL_ERROR_MSG()
		return false;
	}
	return true;
}

bool menu_process(void * arg)
{
	menu_token_t *menu = arg;
	if (menu == NULL)
	{
		NULL_ERROR_MSG();
		return false;
	}

	if (menu->menu_list == NULL || menu->menu_list[menu->position] == NULL) 
	{
		ssd1306_SetCursor(2, MENU_HEIGHT + 2*LINE_HEIGHT);
		ssd1306_WriteString("menu->value == NULL", Font_7x10, White);
		return FALSE;
	}

	ssd1306_Fill(Black);
	ssd1306_SetCursor(2, 0);
	ssd1306_WriteString(menu->name, Font_11x18, White);

	if (menu->line.end - menu->line.start != MAX_LINE - 1)
	{
		menu->line.start = menu->position;
		menu->line.end = menu->line.start + MAX_LINE - 1;
	}

	if (menu->position < menu->line.start || menu->position > menu->line.end)
	{
		if (menu->last_button == LAST_BUTTON_UP)
		{
			menu->line.start = menu->position;
			menu->line.end = menu->line.start + MAX_LINE - 1;
		}
		else
		{
			menu->line.end = menu->position;
			menu->line.start = menu->line.end - MAX_LINE + 1;
		}
		debug_msg("menu->line.start %d, menu->line.end %d, position %d, menu->last_button %d\n", menu->line.start, menu->line.end, menu->position, menu->last_button);
	}

	int line = 0;
	do
	{
		ssd1306_SetCursor(2, MENU_HEIGHT + LINE_HEIGHT*line);
		if (line + menu->line.start == menu->position)
		{
			ssdFigureFillLine(MENU_HEIGHT + LINE_HEIGHT*line, LINE_HEIGHT);
			ssd1306_WriteString(menu->menu_list[line + menu->line.start]->name, Font_7x10, Black);
		}
		else
		{
			ssd1306_WriteString(menu->menu_list[line + menu->line.start]->name, Font_7x10, White);
		}
		line++;
	} while (menu->menu_list[line + menu->line.start] != NULL && line < MAX_LINE);
	scrollBar.actual_line = menu->position;
	scrollBar.all_line = menuDrvElementsCnt(menu) - 1;
	ssdFigureDrawScrollBar(&scrollBar);

	return true;
}

void menuInitDefaultList(menu_token_t *menu)
{
	menu->menu_cb.enter = menu_enter_cb;
	menu->menu_cb.button_init_cb = menu_button_init_cb;
	menu->menu_cb.exit = menu_exit_cb;
	menu->menu_cb.process = menu_process;
}

void mainMenuInit(void)
{
	menuInitDefaultList(&main_menu);
	menuInitDefaultList(&setings);

	menuEnter(&main_menu);
}