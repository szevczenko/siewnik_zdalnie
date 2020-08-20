#include "config.h"
#include "freertos/timers.h"
#include "menu.h"
#include "ssd1306.h"
#include "ssdFigure.h"
#include "but.h"
#include "console.h"
#include "semphr.h"
#include "wifidrv.h"
#include "cmd_client.h"
#include "fast_add.h"

#define debug_msg(...) consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, __VA_ARGS__)

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

menu_token_t * entered_menu_tab[8];
static SemaphoreHandle_t xSemaphore = NULL;
static char wifi_device_list[16][33];
static char devName[33];
static char infoBuff[256];
static uint16_t dev_count;
static TickType_t connect_timeout;
static TickType_t animation_timeout;
static uint8_t animation_cnt;
static TimerHandle_t xTimers;

typedef enum
{
	ST_WIFI_FIND_DEVICE,
	ST_WIFI_DEVICE_LIST,
	ST_WIFI_DEVICE_TRY_CONNECT,
	ST_WIFI_DEVICE_CONNECTED_FAILED,
	ST_WIFI_CONNECT,
}stateWifiMenu_t;
static stateWifiMenu_t wifiState;

uint32_t test_val_ = 43;
uint32_t test_bool_;

uint32_t motor_value;
uint32_t servo_value;
bool motor_on;
bool servo_on;

menu_token_t test_val = 
{
	.name = "test_val1",
	.arg_type = T_ARG_TYPE_VALUE,
	.value = &test_val_
};

menu_token_t test_val1 = 
{
	.name = "test_val2",
	.arg_type = T_ARG_TYPE_VALUE,
	.value = &test_val_
};

menu_token_t test_val2 = 
{
	.name = "test_val3",
	.arg_type = T_ARG_TYPE_VALUE,
	.value = &test_val_
};

menu_token_t test_val3 = 
{
	.name = "test_val4",
	.arg_type = T_ARG_TYPE_VALUE,
	.value = &test_val_
};

menu_token_t test_val4 = 
{
	.name = "test_val5",
	.arg_type = T_ARG_TYPE_VALUE,
	.value = &test_val_
};

menu_token_t test_bool = 
{
	.name = "test_bool",
	.arg_type = T_ARG_TYPE_BOOL,
	.value = (uint32_t *)&test_bool_
};

menu_token_t *setting_tokens[] = {&test_val, &test_bool, &test_val1, &test_val2, &test_val3, &test_val4, NULL};

menu_token_t start_menu = 
{
	.name = "START",
	.arg_type = T_ARG_TYPE_START,
};

menu_token_t wifi_menu = 
{
	.name = "DEVICES",
	.arg_type = T_ARG_TYPE_WIFI,
};

menu_token_t info_menu = 
{
	.name = "INFO",
	.arg_type = T_ARG_TYPE_INFO,
};

menu_token_t setings = 
{
	.name = "SETINGS",
	.arg_type = T_ARG_TYPE_MENU,
	.menu_list = setting_tokens
};

menu_token_t* main_menu_tokens[] = {&start_menu, &setings, &wifi_menu, NULL} ;

menu_token_t main_menu = 
{
	.name = "MENU",
	.arg_type = T_ARG_TYPE_MENU,
	.menu_list = main_menu_tokens
};

static int len_menu(menu_token_t * menu)
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
		if (entered_menu_tab[i] == NULL && i != 0)
			return entered_menu_tab[i - 1];
	}
	return NULL;
}

static int tab_len(void)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		if (entered_menu_tab[i] == NULL)
			return i;
	}
	return 0;
}

static void add_menu_tab(menu_token_t * menu)
{
	int pos = tab_len();
	entered_menu_tab[pos] = menu;
}

static void remove_last_menu_tab(void)
{
	int pos = tab_len();
	if (pos > 1)
		entered_menu_tab[pos - 1] = NULL;
}

static void update_screen(void)
{
	xSemaphoreGive( xSemaphore );
}

void menu_start_find_device_I(void)
{
	if (last_tab_element() != &wifi_menu) return;
	wifiState = ST_WIFI_FIND_DEVICE;
	dev_count = 0;
	uint16_t ap_count = 0;
	char dev_name[33];
	wifiDrvStartScan();
	wifiDrvGetScanResult(&ap_count);
	for (uint16_t i = 0; i < ap_count; i++)
	{
		wifiDrvGetNameFromScannedList(i, dev_name);
		debug_msg("%s\n", dev_name);
		if (memcmp(dev_name, "Zefir", strlen("Zefir")) == 0)
		{
			strcpy(wifi_device_list[dev_count++], dev_name);
		}
	}
	wifiState = ST_WIFI_DEVICE_LIST;
}

void menu_start_find_device(void)
{
	taskEXIT_CRITICAL();
	menu_start_find_device_I();
	taskENTER_CRITICAL();
}

static int connectToDevice(char *dev)
{

	if (memcmp("Zefir", dev, strlen("Zefir")) == 0)
	{
		if (wifiDrvIsConnected()){
			if (wifiDrvDisconnect() != ESP_OK){
				debug_msg("MENU: Disconnect wifi failed");
				menuPrintfInfo("Disconnect wifi failed");
			}
		}
		strcpy(devName, dev);
		debug_msg("MENU: Try connect %s", devName);
		entered_menu_tab[1] = &wifi_menu;
		for (uint8_t i = 2; i < 8; i++)
		{
			entered_menu_tab[i] = NULL;
		}
		wifiDrvSetAPName(dev, strlen(dev));
		wifiDrvSetPassword("12345678", strlen("12345678"));
		if (wifiDrvConnect() != ESP_OK) {
			debug_msg("MENU: wifiDrvConnect failed");
			menuPrintfInfo("wifiDrvConnect failed");
			return FALSE;
		}
		wifiState = ST_WIFI_DEVICE_TRY_CONNECT;
		connect_timeout = xTaskGetTickCount() + MS2ST(3000);
		update_screen();
		return TRUE;
	}
	return FALSE;
}

static void go_to_main_menu(void)
{
	for (uint8_t i = 1; i < 8; i++) {
		entered_menu_tab[i] = NULL;
	}
	update_screen();
}

#define LINE_HEIGHT 10
#define MENU_HEIGHT 18

#define MAX_LINE (SSD1306_HEIGHT - MENU_HEIGHT) / LINE_HEIGHT

static scrollBar_t scrollBar = {
	.line_max = MAX_LINE,
	.y_start = MENU_HEIGHT
};

loadBar_t motor_bar = {
    .x = 40,
    .y = 10,
    .width = 80,
    .height = 10,
};

loadBar_t servo_bar = {
    .x = 40,
    .y = 40,
    .width = 80,
    .height = 10,
};

static int line_start, line_end, last_button;

void button_up_callback(void * arg)
{
	last_button = 1;
	menu_token_t * menu = last_tab_element();
	
	if (menu == NULL) 
	{
		debug_msg("Error button_up_callback: menu == NULL\n");
		return;
	}

	switch(menu->arg_type)
	{
		case T_ARG_TYPE_START:
			update_screen();
			break;

		case T_ARG_TYPE_WIFI:
				if (wifiState == ST_WIFI_DEVICE_CONNECTED_FAILED || (wifiState == ST_WIFI_DEVICE_LIST && dev_count == 0)) {
					menu_start_find_device();
				}
				else if(menu->position > 0) {
					menu->position--;
					update_screen();
				}
			break;

		default:
			if (menu->position > 0) {
				menu->position--;
				update_screen();
			}
			break;
	}
	debug_msg("button_up_callback Len: %d, Pos: %d\n", len_menu(menu), menu->position);
}

void button_down_callback(void * arg)
{
	last_button = 0;
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) 
	{
		debug_msg("Error button_down_callback: menu == NULL\n");
		return;
	}

	switch(menu->arg_type)
	{
		case T_ARG_TYPE_START:
			update_screen();
			break;

		case T_ARG_TYPE_WIFI:
				if (wifiState == ST_WIFI_DEVICE_CONNECTED_FAILED || (wifiState == ST_WIFI_DEVICE_LIST && dev_count == 0)) {
					menu_start_find_device();
				}
				else if (menu->position < dev_count - 1) {
					menu->position++;
					update_screen();
				}	
			break;

		default:
			if (menu->position < len_menu(menu) - 1) {
				menu->position++;
				update_screen();
			}
			break;
	}
	
	
	debug_msg("button_down_callback Len: %d, Pos: %d\n", len_menu(menu), menu->position);
}

void button_plus_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type)
	{
		case T_ARG_TYPE_BOOL:
			if (menu->value == NULL) return;
			if ((bool)*menu->value == (bool)0)
				*menu->value = (bool)1;
		break;

		case T_ARG_TYPE_VALUE:
			if (menu->value == NULL) return;
			if (*menu->value < 100)
				*menu->value = *menu->value + 1;
		break;

		case T_ARG_TYPE_MENU:
			if (menu->menu_list == NULL || menu->menu_list[menu->position] == NULL) return;
			add_menu_tab(menu->menu_list[menu->position]);
			if (last_tab_element() == &wifi_menu)
			{
				debug_msg("Enter wifi_menu\n");
				menu_start_find_device();
			}
		break;

		case T_ARG_TYPE_START:
			if (motor_value < 100) {
				motor_value++;
				cmdClientSetValueWithoutRespI(MENU_MOTOR, motor_value);
			}
				
		break;

		case T_ARG_TYPE_WIFI:
			if (wifiState == ST_WIFI_DEVICE_CONNECTED_FAILED || (wifiState == ST_WIFI_DEVICE_LIST && dev_count == 0)) {
				menu_start_find_device();
			}
			else if (wifiState == ST_WIFI_DEVICE_LIST) {
				connectToDevice(wifi_device_list[menu->position]);
			}
			break;

		default:
			return;
	}
	update_screen();
}

void motor_fast_add(uint32_t value) {
	(void) value;
	cmdClientSetValueWithoutResp(MENU_MOTOR, motor_value);
	//debug_msg("Motor value %d\n", motor_value);
}

void button_plus_time_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type){

		case T_ARG_TYPE_START:
			fastProcessStart(&motor_value, 100, 0, FP_PLUS, motor_fast_add);
		break;

		default:
			return;
	}
	update_screen();
}

void button_plus_up_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type)
	{
		
		case T_ARG_TYPE_START:
			fastProcessStop(&motor_value);
		break;

		default:
			return;
	}
	update_screen();
}

void button_minus_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type)
	{
		case T_ARG_TYPE_BOOL:
			if (menu->value == NULL) return;
			if ((bool)*menu->value == 1)
				*menu->value = (bool)0;
		break;

		case T_ARG_TYPE_VALUE:
			if (menu->value == NULL) return;
			if (*menu->value > 0)
				*menu->value = *menu->value - 1;
		break;

		case T_ARG_TYPE_MENU:
			remove_last_menu_tab();
		break;

		case T_ARG_TYPE_WIFI:
			if (wifiState == ST_WIFI_DEVICE_CONNECTED_FAILED || (wifiState == ST_WIFI_DEVICE_LIST && dev_count == 0)) {
				//menu_start_find_device();
				remove_last_menu_tab();
			}
			else if (wifiState == ST_WIFI_DEVICE_LIST) {
				remove_last_menu_tab();
			}
			break;

		case T_ARG_TYPE_START:
			if (motor_value > 0) {
				motor_value--;
				cmdClientSetValueWithoutRespI(MENU_MOTOR, motor_value);
			}
		break;

		default:
			return;
	}
	update_screen();
}

void button_minus_time_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type){

		case T_ARG_TYPE_START:
			fastProcessStart(&motor_value, 100, 0, FP_MINUS, motor_fast_add);
		break;

		default:
			return;
	}
	update_screen();
}

void button_minus_up_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type)
	{
		
		case T_ARG_TYPE_START:
			fastProcessStop(&motor_value);
		break;

		default:
			return;
	}
	update_screen();
}

void button_enter_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	debug_msg("Enter cur (%s)\n", menu->name);
	switch(menu->arg_type)
	{
		case T_ARG_TYPE_BOOL:
			remove_last_menu_tab();
		break;

		case T_ARG_TYPE_VALUE:
			remove_last_menu_tab();
		break;

		case T_ARG_TYPE_MENU:
			if (menu->menu_list == NULL || menu->menu_list[menu->position] == NULL) return;
			add_menu_tab(menu->menu_list[menu->position]);
			if (last_tab_element() == &wifi_menu)
			{
				debug_msg("Enter wifi_menu\n");
				menu_start_find_device();
			}
		break;

		case T_ARG_TYPE_START:
		break;

		case T_ARG_TYPE_WIFI:
			if (wifiState == ST_WIFI_DEVICE_LIST) {
				connectToDevice(wifi_device_list[menu->position]);
			}
			else if (wifiState == ST_WIFI_DEVICE_CONNECTED_FAILED || (wifiState == ST_WIFI_DEVICE_LIST && dev_count == 0)) {
				menu_start_find_device();
			}
		break;

		default:
			return;
	}
	update_screen();
}

void button_exit_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	debug_msg("Exit\n", len_menu(menu), menu->position);
	if (menu == NULL) return;
	switch(menu->arg_type)
	{
		case T_ARG_TYPE_BOOL:
			remove_last_menu_tab();
		break;

		case T_ARG_TYPE_VALUE:
			remove_last_menu_tab();
		break;

		case T_ARG_TYPE_MENU:
			remove_last_menu_tab();
		break;

		case T_ARG_TYPE_START:
			remove_last_menu_tab();
		break;

		case T_ARG_TYPE_WIFI:
			remove_last_menu_tab();
		break;

		default:
			return;
	}
	update_screen();
}

static void menu_task(void * arg)
{
	(void) arg;
	menu_token_t * menu;
	while(1)
	{
		xSemaphoreTake( xSemaphore, ( TickType_t ) MS2ST(100) );
		//taskENTER_CRITICAL();
		menu = last_tab_element();
		if (menu == NULL) 
		{
			debug_msg("MENU_ERROR: menu_task\n");
			taskEXIT_CRITICAL();
			vTaskDelay(MS2ST(100));
			continue;
		}
		ssd1306_Fill(Black);
		ssd1306_SetCursor(2, 0);
		ssd1306_WriteString(menu->name, Font_11x18, White);
		switch(menu->arg_type)
		{
			case T_ARG_TYPE_BOOL:
				if (menu->value == NULL) return;
				ssd1306_SetCursor(50, MENU_HEIGHT + 5);
				if (*((bool*)menu->value) == 1)
					ssd1306_WriteString("ON", Font_11x18, White);
				else
					ssd1306_WriteString("OFF", Font_11x18, White);
				ssd1306_SetCursor(2, MENU_HEIGHT + 3*LINE_HEIGHT);
				ssd1306_WriteString("+/- For edit", Font_7x10, White);
			break;

			case T_ARG_TYPE_VALUE:
				if (menu->value == NULL) return;
				{
					ssd1306_SetCursor(55, MENU_HEIGHT + 5);
					char buff[8];
					sprintf(buff, "%d", *menu->value);
					ssd1306_WriteString(buff, Font_11x18, White);
					ssd1306_SetCursor(2, MENU_HEIGHT + 3*LINE_HEIGHT - 1);
					ssd1306_WriteString("+/- For edit", Font_7x10, White);
				}
			break;

			case T_ARG_TYPE_MENU:
				if (menu->menu_list == NULL || menu->menu_list[menu->position] == NULL) return;
				{
					if (line_end - line_start != MAX_LINE - 1)
					{
						line_start = menu->position;
						line_end = line_start + MAX_LINE - 1;
					}
					if (menu->position < line_start || menu->position > line_end)
					{
						if (last_button)
						{
							line_start = menu->position;
							line_end = line_start + MAX_LINE - 1;
						}
						else
						{
							line_end = menu->position;
							line_start = line_end - MAX_LINE + 1;
						}
					}
					//debug_msg("line_start %d, line_end %d, position %d, last_button %d\n", line_start, line_end, menu->position, last_button);
					int line = 0;
					do
					{
						ssd1306_SetCursor(2, MENU_HEIGHT + LINE_HEIGHT*line);
						if (line + line_start == menu->position)
						{
							ssdFigureFillLine(MENU_HEIGHT + LINE_HEIGHT*line, LINE_HEIGHT);
							ssd1306_WriteString(menu->menu_list[line + line_start]->name, Font_7x10, Black);
						}
						else
						{
							ssd1306_WriteString(menu->menu_list[line + line_start]->name, Font_7x10, White);
						}
						
						line++;
					} while (menu->menu_list[line + line_start] != NULL && line < MAX_LINE);
					scrollBar.actual_line = menu->position;
					scrollBar.all_line = len_menu(menu) - 1;
					ssdFigureDrawScrollBar(&scrollBar);
				}
			break;

			case T_ARG_TYPE_START:
				if (!wifiDrvIsConnected()) {
					remove_last_menu_tab();
					if (wifiState == ST_WIFI_CONNECT) {
						menuPrintfInfo("Lost connection with target");
					}
					else {
						menuPrintfInfo("Target not connected. Go to DEVICES and connect to target");
					}
				}
				else { 
					if (animation_timeout < xTaskGetTickCount()) {
						animation_cnt++;
						animation_timeout = xTaskGetTickCount() + MS2ST(100);
					}
					char str[8];
					
					motor_bar.fill = motor_value;
					sprintf(str, "%d", motor_bar.fill);
					ssd1306_Fill(Black);
					ssdFigureDrawLoadBar(&motor_bar);
					ssd1306_SetCursor(80, 25);
					ssd1306_WriteString(str, Font_7x10, White);
					uint8_t cnt = 0;
					if (motor_on) {
						cnt = animation_cnt % 8;
					}
					if (cnt < 4) {
						if (cnt < 2) {
							drawMotor(2, 2 - cnt);
						}
						else {
							drawMotor(2, cnt - 2);
						}
					}
					else
					{
						if (cnt < 6) {
							drawMotor(2, cnt - 2);
						}
						else {
							drawMotor(2, 10 - cnt);
						}
					}

					servo_bar.fill = servo_value;
					sprintf(str, "%d", servo_bar.fill);
					ssdFigureDrawLoadBar(&servo_bar);
					ssd1306_SetCursor(80, 55);
					ssd1306_WriteString(str, Font_7x10, White);
					if (servo_on) {
						drawServo(10, 35, servo_value);
					}
					else {
						drawServo(10, 35, 0);
					}
					
				}
				
			break;

			case T_ARG_TYPE_WIFI:
				if (wifiState == ST_WIFI_DEVICE_LIST)
				{
					if (dev_count == 0)
					{
						ssd1306_SetCursor(2, MENU_HEIGHT + LINE_HEIGHT);
						ssd1306_WriteString("Devices not found.", Font_7x10, White);
						ssd1306_SetCursor(2, MENU_HEIGHT + 2*LINE_HEIGHT);
						ssd1306_WriteString("Click button for", Font_7x10, White);
						ssd1306_SetCursor(2, MENU_HEIGHT + 3*LINE_HEIGHT);
						ssd1306_WriteString("try found device", Font_7x10, White);
						break;
					}
					if (line_end - line_start != MAX_LINE - 1)
					{
						line_start = menu->position;
						line_end = line_start + MAX_LINE - 1;
					}
					if (menu->position < line_start || menu->position > line_end)
					{
						if (last_button)
						{
							line_start = menu->position;
							line_end = line_start + MAX_LINE - 1;
						}
						else
						{
							line_end = menu->position;
							line_start = line_end - MAX_LINE + 1;
						}
					}
					//debug_msg("position %d, dev_count %d\n", menu->position, dev_count);
					int line = 0;
					do
					{
						ssd1306_SetCursor(2, MENU_HEIGHT + LINE_HEIGHT*line);
						if (line + line_start == menu->position)
						{
							ssdFigureFillLine(MENU_HEIGHT + LINE_HEIGHT*line, LINE_HEIGHT);
							ssd1306_WriteString(wifi_device_list[line + line_start], Font_7x10, Black);
						}
						else
						{
							ssd1306_WriteString(wifi_device_list[line + line_start], Font_7x10, White);
						}
						
						line++;
					} while (line + line_start < dev_count - 1 && line < MAX_LINE);
					scrollBar.actual_line = menu->position;
					scrollBar.all_line = dev_count - 1;
					ssdFigureDrawScrollBar(&scrollBar);
				}
				else if (wifiState == ST_WIFI_FIND_DEVICE)
				{
					ssd1306_SetCursor(2, MENU_HEIGHT + LINE_HEIGHT);
					ssd1306_WriteString("Scanning...", Font_7x10, White);
					break;
				}
				else if (wifiState == ST_WIFI_DEVICE_TRY_CONNECT)
				{
					if (wifiDrvIsConnected() == 1)
					{
						wifiState = ST_WIFI_CONNECT;
					}
					else
					{
						if (connect_timeout < xTaskGetTickCount())
						{
							wifiState = ST_WIFI_DEVICE_CONNECTED_FAILED;
						}
						ssd1306_SetCursor(2, MENU_HEIGHT + LINE_HEIGHT);
						ssd1306_WriteString("Try connect to", Font_7x10, White);
						ssd1306_SetCursor(2, MENU_HEIGHT + 2*LINE_HEIGHT);
						ssd1306_WriteString(devName, Font_7x10, White);
					}
					
				}
				else if (wifiState == ST_WIFI_DEVICE_CONNECTED_FAILED)
				{
					ssd1306_SetCursor(2, MENU_HEIGHT + LINE_HEIGHT);
					ssd1306_WriteString("Connected failed.", Font_7x10, White);
					ssd1306_SetCursor(2, MENU_HEIGHT + 2*LINE_HEIGHT);
					ssd1306_WriteString("Click button for", Font_7x10, White);
					ssd1306_SetCursor(2, MENU_HEIGHT + 3*LINE_HEIGHT);
					ssd1306_WriteString("try found device", Font_7x10, White);
				}
				else if (wifiState == ST_WIFI_CONNECT)
				{
					go_to_main_menu();
					menuPrintfInfo("Connected");
					break;
				}
			break;

			case T_ARG_TYPE_INFO:
				{
					int line = 0;
					ssd1306_SetCursor(2, MENU_HEIGHT + LINE_HEIGHT*line);
					for (int i = 0; i < strlen(infoBuff); i++)
					{
						if (i * 7 >= line * SSD1306_WIDTH + SSD1306_WIDTH)
						{
							line++;
							ssd1306_SetCursor(2, MENU_HEIGHT + LINE_HEIGHT*line);
						}
						ssd1306_WriteChar(infoBuff[i], Font_7x10, White);
					}
				}
				ssd1306_UpdateScreen();
				vTaskDelay(MS2ST(1500));
				menuActivateButtons();
				remove_last_menu_tab();
				break;

			default:
				return;
		}
		ssd1306_UpdateScreen();
		//taskEXIT_CRITICAL();
	}
}

void menuPrintfInfo(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vsnprintf(infoBuff, sizeof(infoBuff), format, ap);
	va_end(ap);
	add_menu_tab(&info_menu);
}

void button_plus_servo_up_callback(void * arg);
void button_plus_servo_callback(void * arg);
void button_plus_servo_time_callback(void * arg);

void button_minus_servo_up_callback(void * arg);
void button_minus_servo_callback(void * arg);
void button_minus_servo_time_callback(void * arg);

void button_motor_state(void * arg);
void button_servo_state(void * arg);

void menuDeactivateButtons(void)
{
	button1.fall_callback = NULL;
	button2.fall_callback = NULL;
	button3.fall_callback = NULL;
	button4.fall_callback = NULL;
	button4.timer_callback = NULL;
	button4.rise_callback = NULL;
	button5.fall_callback = NULL;
	button6.fall_callback = NULL;
	button6.timer_callback = NULL;
	button6.rise_callback = NULL;
	button7.fall_callback = NULL;
	button7.timer_callback = NULL;
	button7.rise_callback = NULL;
	button9.fall_callback = NULL;
	button10.fall_callback = NULL;
}

void menuActivateButtons(void)
{
	button1.fall_callback = button_up_callback;
	button2.fall_callback = button_down_callback;
	button6.fall_callback = button_plus_callback;
	button6.timer_callback = button_plus_time_callback;
	button6.rise_callback = button_plus_up_callback;
	button4.fall_callback = button_minus_callback;
	button4.timer_callback = button_minus_time_callback;
	button4.rise_callback = button_minus_up_callback;
	button7.fall_callback = button_plus_servo_callback;
	button7.timer_callback = button_plus_servo_time_callback;
	button7.rise_callback = button_plus_servo_up_callback;
	button5.fall_callback = button_minus_servo_callback;
	button5.timer_callback = button_minus_servo_time_callback;
	button5.rise_callback = button_minus_servo_up_callback;
	button3.fall_callback = button_enter_callback;
	button8.fall_callback = button_exit_callback;
	button9.fall_callback = button_motor_state;
	button10.fall_callback = button_servo_state;
}

static void timerCallback(void * pv) {
	servo_on = 1;
	cmdClientSetValueWithoutResp(MENU_SERVO_IS_ON, servo_on);
}

void init_menu(void)
{
	entered_menu_tab[0] = &main_menu;
	xSemaphore = xSemaphoreCreateBinary();
	xTimers = xTimerCreate("Timer", MS2ST(2000), pdTRUE, ( void * ) 0, timerCallback);
	xTaskCreate(menu_task, "menu_task", 2048, NULL, 10, NULL);
	wifiDrvGetAPName(devName);
	if (connectToDevice(devName) == FALSE)
	{
		entered_menu_tab[1] = &wifi_menu;
		menu_start_find_device_I();
	}
	menuActivateButtons();
	update_screen();
}

void button_plus_servo_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type)
	{
	
		case T_ARG_TYPE_START:
			if (servo_value < 100) {
				servo_value++;
				cmdClientSetValueWithoutRespI(MENU_SERVO, servo_value);
			}
		break;

		default:
			return;
	}
	update_screen();
}

static void servo_fast_add(uint32_t value) {
	(void) value;
	cmdClientSetValueWithoutResp(MENU_SERVO, servo_value);
	//debug_msg("servo_value %d\n", servo_value);
}

void button_plus_servo_time_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type){

		case T_ARG_TYPE_START:
			fastProcessStart(&servo_value, 100, 0, FP_PLUS, servo_fast_add);
		break;

		default:
			return;
	}
	update_screen();
}

void button_plus_servo_up_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type)
	{
		
		case T_ARG_TYPE_START:
			fastProcessStop(&servo_value);
		break;

		default:
			return;
	}
	update_screen();
}

void button_minus_servo_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type)
	{
	
		case T_ARG_TYPE_START:
			if (servo_value > 0) {
				servo_value--;
				cmdClientSetValueWithoutRespI(MENU_SERVO, servo_value);
			}
		break;

		default:
			return;
	}
	update_screen();
}

void button_minus_servo_time_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type){

		case T_ARG_TYPE_START:
			fastProcessStart(&servo_value, 100, 0, FP_MINUS, servo_fast_add);
		break;

		default:
			return;
	}
	update_screen();
}

void button_minus_servo_up_callback(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type)
	{
		
		case T_ARG_TYPE_START:
			fastProcessStop(&servo_value);
		break;

		default:
			return;
	}
	update_screen();
}

void button_motor_state(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type)
	{
		
		case T_ARG_TYPE_START:
			if (motor_on) {
				motor_on = 0;
				servo_on = 0;
				cmdClientSetValueWithoutRespI(MENU_MOTOR_IS_ON, motor_on);
				cmdClientSetValueWithoutRespI(MENU_SERVO_IS_ON, servo_on);
				xTimerStop(xTimers, 0);
			}
			else {
				xTimerStart(xTimers, 0);
				motor_on = 1;
				cmdClientSetValueWithoutRespI(MENU_MOTOR_IS_ON, motor_on);
			}
		break;

		default:
			return;
	}
	update_screen();
}

void button_servo_state(void * arg)
{
	menu_token_t * menu = last_tab_element();
	if (menu == NULL) return;
	switch(menu->arg_type)
	{
		
		case T_ARG_TYPE_START:
			xTimerStop(xTimers, 0);
			if (servo_on) {
				servo_on = 0;
				cmdClientSetValueWithoutRespI(MENU_SERVO_IS_ON, servo_on);
			}
			else {
				servo_on = 1;
				cmdClientSetValueWithoutRespI(MENU_SERVO_IS_ON, servo_on);
			}
		break;

		default:
			return;
	}
	update_screen();
}


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif