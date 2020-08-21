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
#include "atmega_communication.h"
#include "error_siewnik.h"
#include "measure.h"

#define debug_msg(...) consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, __VA_ARGS__)

uint16_t test_value;

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
    }
    else {
        at_communication_init();
        errorSiewnikStart();
        measure_start();
    }
    
    while(1)
    {
        vTaskDelay(MS2ST(1000));
        if (config.dev_type != T_DEV_TYPE_SERVER)
        {
           
        }
    }
}