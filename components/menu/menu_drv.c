#include "stdint.h"

#include "config.h"
#include "menu.h"
#include "menu_drv.h"
#include "ssd1306.h"
#include "ssdFigure.h"
#include "but.h"
#include "semphr.h"
#include "menu_param.h"

#define CONFIG_MENU_TEST_TASK 	0
#define LINE_HEIGHT 			10
#define MENU_HEIGHT 			18
#define MAX_LINE 				(SSD1306_HEIGHT - MENU_HEIGHT) / LINE_HEIGHT

typedef enum
{
	MENU_STATE_INIT,
	MENU_STATE_IDLE,
	MENU_STATE_ENTER,
	MENU_STATE_EXIT,
	MENU_STATE_PROCESS,
	MENU_STATE_INFO,
	MENU_STATE_ERROR_CHECK,
	MENU_STATE_TOP
}menu_state_t;

typedef struct 
{
	menu_state_t state;
	menu_token_t * entered_menu_tab[8];
	TickType_t save_timeout;
	bool save_flag;
	bool exit_req;
	bool enter_req;
	bool error_flag;
	int error_code;
	char * error_msg;
	menu_token_t *new_menu;
	SemaphoreHandle_t update_screen_req;
} menu_drv_t;

static menu_drv_t ctx;

extern void mainMenuInit(void);

static char *state_name[] = 
{
	[MENU_STATE_INIT] = "MENU_STATE_INIT",
	[MENU_STATE_IDLE] = "MENU_STATE_IDLE",
	[MENU_STATE_ENTER] = "MENU_STATE_ENTER",
	[MENU_STATE_EXIT] = "MENU_STATE_EXIT",
	[MENU_STATE_PROCESS] = "MENU_STATE_PROCESS",
	[MENU_STATE_INFO] = "MENU_STATE_INFO",
	[MENU_STATE_ERROR_CHECK] = "MENU_STATE_ERROR_CHECK",
};

static but_t button1_menu, button2_menu, button3_menu, button4_menu, button5_menu, button6_menu, button7_menu, button8_menu, button9_menu, button10_menu;

static void update_screen(void)
{
	xSemaphoreGive( ctx.update_screen_req );
}

static void save_parameters(void) {
	ctx.save_timeout = xTaskGetTickCount() + MS2ST(1000);
	ctx.save_flag = 1;
}

static void save_process(void) {
	if (ctx.save_flag == 1 && ctx.save_timeout < xTaskGetTickCount()) {
		menuSaveParameters();
		ctx.save_flag = 0;
	}
}

int menuDrvElementsCnt(menu_token_t * menu)
{
	if (menu->menu_list == NULL)
	{
		debug_msg("menu->menu_list == NULL (%s)\n", menu->name);
		return 0;
	}
	int len = 0;
	menu_token_t ** actual_token = menu->menu_list;
	do
	{
		if (actual_token[len] == NULL)
		{
			return len;
		}
		len++;
	} while (len < 255);
	return 0;
}

static menu_token_t * last_tab_element(void)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		if (ctx.entered_menu_tab[i] == NULL && i != 0)
			return ctx.entered_menu_tab[i - 1];
	}
	return NULL;
}

static int tab_len(void)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		if (ctx.entered_menu_tab[i] == NULL)
			return i;
	}
	return 0;
}

static void add_menu_tab(menu_token_t * menu)
{
	int pos = tab_len();
	ctx.entered_menu_tab[pos] = menu;
}

static void remove_last_menu_tab(void)
{
	int pos = tab_len();
	if (pos > 1)
		ctx.entered_menu_tab[pos - 1] = NULL;
}

static void go_to_main_menu(void)
{
	for (uint8_t i = 1; i < 8; i++) {
		ctx.entered_menu_tab[i] = NULL;
	}
	update_screen();
}

static void menu_fall_callback_but_cb(void * arg)
{
	but_t * button = (but_t*)arg;
	if (button->fall_callback != NULL)
	{
		button->fall_callback(button->arg);
		update_screen();
	}
}

static void menu_rise_callback_but_cb(void * arg)
{
	but_t * button = (but_t*)arg;
	if (button->rise_callback != NULL)
	{
		button->rise_callback(button->arg);
		update_screen();
	}
}

static void menu_timer_callback_but_cb(void * arg)
{
	but_t * button = (but_t*)arg;
	if (button->timer_callback != NULL)
	{
		button->timer_callback(button->arg);
		update_screen();
	}
}

static void menu_init_buttons(void)
{
	button1.arg = &button1_menu;
	button1.fall_callback = menu_fall_callback_but_cb;
	button1.rise_callback = menu_rise_callback_but_cb;
	button1.timer_callback = menu_timer_callback_but_cb;

	button2.arg = &button2_menu;
	button2.fall_callback = menu_fall_callback_but_cb;
	button2.rise_callback = menu_rise_callback_but_cb;
	button2.timer_callback = menu_timer_callback_but_cb;

	button3.arg = &button3_menu;
	button3.fall_callback = menu_fall_callback_but_cb;
	button3.rise_callback = menu_rise_callback_but_cb;
	button3.timer_callback = menu_timer_callback_but_cb;

	button4.arg = &button4_menu;
	button4.fall_callback = menu_fall_callback_but_cb;
	button4.rise_callback = menu_rise_callback_but_cb;
	button4.timer_callback = menu_timer_callback_but_cb;

	button5.arg = &button5_menu;
	button5.fall_callback = menu_fall_callback_but_cb;
	button5.rise_callback = menu_rise_callback_but_cb;
	button5.timer_callback = menu_timer_callback_but_cb;

	button6.arg = &button6_menu;
	button6.fall_callback = menu_fall_callback_but_cb;
	button6.rise_callback = menu_rise_callback_but_cb;
	button6.timer_callback = menu_timer_callback_but_cb;

	button7.arg = &button7_menu;
	button7.fall_callback = menu_fall_callback_but_cb;
	button7.rise_callback = menu_rise_callback_but_cb;
	button7.timer_callback = menu_timer_callback_but_cb;

	button8.arg = &button8_menu;
	button8.fall_callback = menu_fall_callback_but_cb;
	button8.rise_callback = menu_rise_callback_but_cb;
	button8.timer_callback = menu_timer_callback_but_cb;

	button9.arg = &button9_menu;
	button9.fall_callback = menu_fall_callback_but_cb;
	button9.rise_callback = menu_rise_callback_but_cb;
	button9.timer_callback = menu_timer_callback_but_cb;

	button10.arg = &button10_menu;
	button10.fall_callback = menu_fall_callback_but_cb;
	button10.rise_callback = menu_rise_callback_but_cb;
	button10.timer_callback = menu_timer_callback_but_cb;
}

static void menu_activate_but(menu_token_t * menu)
{
	button1_menu.arg = (void*) menu;
	button1_menu.fall_callback = menu->button.up.fall_callback;
	button1_menu.rise_callback = menu->button.up.rise_callback;
	button1_menu.timer_callback = menu->button.up.timer_callback;

	button2_menu.arg = (void*) menu;
	button2_menu.fall_callback = menu->button.down.fall_callback;
	button2_menu.rise_callback = menu->button.down.rise_callback;
	button2_menu.timer_callback = menu->button.down.timer_callback;

	button3_menu.arg = (void*) menu;
	button3_menu.fall_callback = menu->button.enter.fall_callback;
	button3_menu.rise_callback = menu->button.enter.rise_callback;
	button3_menu.timer_callback = menu->button.enter.timer_callback;

	button4_menu.arg = (void*) menu;
	button4_menu.fall_callback = menu->button.up_minus.fall_callback;
	button4_menu.rise_callback = menu->button.up_minus.rise_callback;
	button4_menu.timer_callback = menu->button.up_minus.timer_callback;

	button5_menu.arg = (void*) menu;
	button5_menu.fall_callback = menu->button.up_plus.fall_callback;
	button5_menu.rise_callback = menu->button.up_plus.rise_callback;
	button5_menu.timer_callback = menu->button.up_plus.timer_callback;

	button6_menu.arg = (void*) menu;
	button6_menu.fall_callback = menu->button.down_minus.fall_callback;
	button6_menu.rise_callback = menu->button.down_minus.rise_callback;
	button6_menu.timer_callback = menu->button.down_minus.timer_callback;

	button7_menu.arg = (void*) menu;
	button7_menu.fall_callback = menu->button.down_plus.fall_callback;
	button7_menu.rise_callback = menu->button.down_plus.rise_callback;
	button7_menu.timer_callback = menu->button.down_plus.timer_callback;

	button8_menu.arg = (void*) menu;
	button8_menu.fall_callback = menu->button.exit.fall_callback;
	button8_menu.rise_callback = menu->button.exit.rise_callback;
	button8_menu.timer_callback = menu->button.exit.timer_callback;

	button9_menu.arg = (void*) menu;
	button9_menu.fall_callback = menu->button.motor_on.fall_callback;
	button9_menu.rise_callback = menu->button.motor_on.rise_callback;
	button9_menu.timer_callback = menu->button.motor_on.timer_callback;

	button10_menu.arg = (void*) menu;
	button10_menu.fall_callback = menu->button.on_off.fall_callback;
	button10_menu.rise_callback = menu->button.on_off.rise_callback;
	button10_menu.timer_callback = menu->button.on_off.timer_callback;
}

void menu_deactivate_but(void)
{
	button1_menu.fall_callback = NULL;
	button1_menu.rise_callback = NULL;
	button1_menu.timer_callback = NULL;

	button2_menu.fall_callback = NULL;
	button2_menu.rise_callback = NULL;
	button2_menu.timer_callback = NULL;

	button3_menu.fall_callback = NULL;
	button3_menu.rise_callback = NULL;
	button3_menu.timer_callback = NULL;

	button4_menu.fall_callback = NULL;
	button4_menu.rise_callback = NULL;
	button4_menu.timer_callback = NULL;

	button5_menu.fall_callback = NULL;
	button5_menu.rise_callback = NULL;
	button5_menu.timer_callback = NULL;

	button6_menu.fall_callback = NULL;
	button6_menu.rise_callback = NULL;
	button6_menu.timer_callback = NULL;

	button7_menu.fall_callback = NULL;
	button7_menu.rise_callback = NULL;
	button7_menu.timer_callback = NULL;

	button8_menu.fall_callback = NULL;
	button8_menu.rise_callback = NULL;
	button8_menu.timer_callback = NULL;

	button9.fall_callback = NULL;
	button9.rise_callback = NULL;
	button9.timer_callback = NULL;

	button10.fall_callback = NULL;
	button10.rise_callback = NULL;
	button10.timer_callback = NULL;
}

static void menu_state_init(void)
{
	ssd1306_SetCursor(2, MENU_HEIGHT + 2*LINE_HEIGHT);
	ssd1306_WriteString("Wait to init...", Font_7x10, White);
	ssd1306_UpdateScreen();
	menu_init_buttons();
	ctx.state = MENU_STATE_IDLE;
}

static void menu_state_idle(menu_token_t * menu)
{
	if (ctx.enter_req)
	{
		if (ctx.new_menu == NULL)
		{
			ctx.error_flag = true;
			ctx.error_msg = "Menu is NULL";
			ctx.state = MENU_STATE_ERROR_CHECK;
			ctx.enter_req = false;
			return;
		}
		add_menu_tab(ctx.new_menu);
		ctx.state = MENU_STATE_ENTER;
		ctx.enter_req = false;
		return;
	}

	if (menu == NULL)
	{
		ssd1306_SetCursor(2, MENU_HEIGHT + 2*LINE_HEIGHT);
		ssd1306_WriteString("Menu idle state...", Font_7x10, White);
		ssd1306_UpdateScreen();
		osDelay(100);
	}
	else
	{
		ctx.state = MENU_STATE_ENTER;
		ctx.enter_req = false;
	}
}

static void menu_state_enter(menu_token_t * menu)
{
	if (menu->menu_cb.button_init_cb != NULL)
	{
		menu->menu_cb.button_init_cb(menu);
	}

	menu_deactivate_but();
	menu_activate_but(menu);

	if (menu->menu_cb.enter != NULL)
	{
		menu->menu_cb.enter(menu);
	}
	ctx.state = MENU_STATE_PROCESS;
}

static void menu_state_process(menu_token_t * menu)
{
	xSemaphoreTake(ctx.update_screen_req, ( TickType_t ) MS2ST(250));
	ssd1306_Fill(Black);
	if (menu->menu_cb.process != NULL)
	{
		menu->menu_cb.process((void *)menu);
	}
	else
	{
		ctx.error_flag = true;
		ctx.error_msg = "Process cb NULL";
		ctx.state = MENU_STATE_EXIT;
	}
	ssd1306_UpdateScreen();

	if (ctx.enter_req || ctx.exit_req)
	{
		ctx.state = MENU_STATE_EXIT;
	}
}

static void menu_state_exit(menu_token_t * menu)
{
	menu_deactivate_but();

	if (menu->menu_cb.exit != NULL)
	{
		menu->menu_cb.exit(menu);
	}
	
	if (ctx.exit_req)
	{
		remove_last_menu_tab();
		ctx.exit_req = false;
	}
	ctx.state = MENU_STATE_ERROR_CHECK;
}

static void menu_state_error_check(menu_token_t *menu)
{
	static char buff[128];
	if (ctx.error_flag)
	{
		ssd1306_Fill(Black);
		if (menu != NULL)
		{
			ssd1306_SetCursor(2, 0);
			ssd1306_WriteString(menu->name, Font_11x18, White);
		}
		ssd1306_SetCursor(2, MENU_HEIGHT);
		ssd1306_WriteString("Error menu_drv", Font_7x10, White);
		ssd1306_SetCursor(2, MENU_HEIGHT + LINE_HEIGHT);
		sprintf(buff, "Error %d...", ctx.error_code);
		ssd1306_WriteString(buff, Font_7x10, White);
		ssd1306_SetCursor(2, MENU_HEIGHT + 2*LINE_HEIGHT);

		if (ctx.error_msg != NULL)
		{
			sprintf(buff, "Msg: %s", ctx.error_msg);
			ssd1306_WriteString(buff, Font_7x10, White);
		}
		else
		{
			ssd1306_WriteString("Undeff", Font_7x10, White);
		}
		
		ssd1306_UpdateScreen();
		ctx.error_flag = false;
		ctx.error_code = 0;
		ctx.error_msg = NULL;
		ctx.exit_req = true;
		osDelay(1250);
	}
	ctx.state = MENU_STATE_IDLE;
}

static void menu_task(void * arg)
{
	menu_token_t *menu = NULL;
	int prev_state = -1;

	while(1)
	{
		menu = last_tab_element();
		// if (prev_state != ctx.state)
		// {
		// 	if (menu != NULL)
		// 	{
		// 		debug_msg("state: %s, menu %s\n\r", state_name[ctx.state], menu->name);
		// 	}
		// 	else
		// 	{
		// 		debug_msg("state %s, menu is NULL\n\r", state_name[ctx.state]);
		// 	}
		// 	prev_state = ctx.state;
		// 	osDelay(10);
		// }
		
		switch(ctx.state)
		{
			case MENU_STATE_INIT:
				menu_state_init();
				break;
			
			case MENU_STATE_IDLE:
				menu_state_idle(menu);
				break;

			case MENU_STATE_ENTER:
				menu_state_enter(menu);
				break;
			
			case MENU_STATE_EXIT:
				menu_state_exit(menu);
				break;
			
			case MENU_STATE_PROCESS:
				menu_state_process(menu);
				break;

			case MENU_STATE_INFO:
				
				break;

			case MENU_STATE_ERROR_CHECK:
				menu_state_error_check(menu);
				break;

			default:
				ctx.state = MENU_STATE_IDLE;
				break;

		}
	}
}

void menuEnter(menu_token_t * menu)
{
	ctx.new_menu = menu;
	ctx.enter_req = true;
	update_screen();
}

void menuExit(menu_token_t * menu)
{
	ctx.exit_req = true;
	update_screen();
}

void init_menu(void)
{
	ctx.update_screen_req = xSemaphoreCreateBinary();
	#if CONFIG_MENU_TEST_TASK
	xTaskCreate(menu_test, "menu_test", 4096, NULL, 12, NULL);
	#else
	xTaskCreate(menu_task, "menu_task", 4096, NULL, 12, NULL);
	#endif
	update_screen();
	mainMenuInit();
}