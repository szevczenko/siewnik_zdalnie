/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "config.h"
#include "console.h"
#include "wifidrv.h"
#include "cmd_server.h"
#include "but.h"
#include "fast_add.h"
#include "ssd1306.h"
#include "ssd1306_tests.h"
#include "menu.h"
#include "keepalive.h"
#include "menu_param.h"
#include "cmd_client.h"
#define debug_msg(...) consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, __VA_ARGS__)

uint8_t test_value;
uint8_t test_value_2 = 10;

void test_func(uint8_t value)
{
    consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, "value %d\n", value);
}

void bytton_rise_callback(void * arg)
{
    fastProcessStop(&test_value);
    fastProcessStop(&test_value_2);
    consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, "bytton_rise_callback\n");
}

void bytton_fall_callback(void * arg)
{
    test_value++;
    consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, "bytton_fall_callback %d\n", test_value);
}

void bytton_time_callback(void * arg)
{
    fastProcessStart(&test_value, 100, 0, FP_PLUS, test_func);
    fastProcessStart(&test_value_2, 100, 0, FP_PLUS, test_func);
    consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, "bytton_time_callback\n");
}

void bytton2_rise_callback(void * arg)
{
    fastProcessStop(&test_value);
    fastProcessStop(&test_value_2);
    consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, "bytton2_rise_callback\n");
}

void bytton2_fall_callback(void * arg)
{
    test_value--;
    consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, "bytton2_fall_callback %d\n", test_value);
}

void bytton2_time_callback(void * arg)
{
    fastProcessStart(&test_value, 100, 0, FP_MINUS, test_func);
    fastProcessStart(&test_value_2, 100, 0, FP_MINUS, test_func);
    consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, "bytton2_time_callback\n");
}


void app_main()
{
    configInit();
    consoleStart();
    wifiDrvInit();
    keepAliveStartTask();
    menuParamInit();
    if (config.dev_type != T_DEV_TYPE_SERVER)
    {
        init_buttons();
        fastProcessStartTask();
        ssd1306_Init();
        init_menu();
        // button1.rise_callback = bytton_rise_callback;
        // button1.fall_callback = bytton_fall_callback;
        // button1.timer_callback = bytton_time_callback;
        // button2.rise_callback = bytton2_rise_callback;
        // button2.fall_callback = bytton2_fall_callback;
        // button2.timer_callback = bytton2_time_callback;
        // ssd1306_TestAll();
    }
    
    while(1)
    {
        vTaskDelay(MS2ST(50));
        if (config.dev_type != T_DEV_TYPE_SERVER)
        {
           if(cmdClientSetValueWithoutResp(MENU_MOTOR, 50) == 1) {
               //debug_msg("Positive\n");
           }
           else {
               //debug_msg("MENU_MOTOR Negative\n");
           }

           if(cmdClientSetValueWithoutResp(MENU_SERVO, 80)) {
               //debug_msg("MENU_SERVO Positive\n");
           }
           else {
               //debug_msg("MENU_SERVO Negative\n");
           }
        }
    }
}