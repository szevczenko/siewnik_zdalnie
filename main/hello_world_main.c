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
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "pcf8574.h"
#include "battery.h"


uint16_t test_value;
static gpio_config_t io_conf;

static int i2cInit(void)
{
    int i2c_master_port = SSD1306_I2C_PORT;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_EXAMPLE_MASTER_SDA_IO;
    conf.sda_pullup_en = 1;
    conf.scl_io_num = I2C_EXAMPLE_MASTER_SCL_IO;
    conf.scl_pullup_en = 1;
    conf.clk_stretch_tick = 1000; // 300 ticks, Clock stretch is about 210us, you can make changes according to the actual situation.
    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode));
    ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));
    return ESP_OK;
}



static void checkDevType(void) {
    i2cInit();
    pcf8574_init();
    int read_i2c_value;
    read_i2c_value = pcf8574_getinput(0);
    if (read_i2c_value >= 0) {
        config.dev_type = T_DEV_TYPE_CLIENT;
    }
    else {
        config.dev_type = T_DEV_TYPE_SERVER;
    }
}

void app_main()
{
    configInit();
    checkDevType();

    consoleStart();
    wifiDrvInit();
    keepAliveStartTask();
    menuParamInit();

    if (config.dev_type != T_DEV_TYPE_SERVER)
    {
        battery_init();
        init_buttons();
        fastProcessStartTask();
        ssd1306_Init();
        init_menu();
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (1 << GPIO_NUM_2);
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;
        gpio_config(&io_conf);
    }
    else {
        at_communication_init();
        errorSiewnikStart();
        measure_start();
        //LED on
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (1 << GPIO_NUM_2);
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;
        gpio_config(&io_conf);
    }

    while(1)
    {
        vTaskDelay(MS2ST(975));
        //if (config.dev_type == T_DEV_TYPE_SERVER)
        {
           gpio_set_level(GPIO_NUM_2, 0);
        }
        
        vTaskDelay(MS2ST(25));

        //if (config.dev_type == T_DEV_TYPE_SERVER)
        {
           gpio_set_level(GPIO_NUM_2, 1);
        }
        if (config.dev_type != T_DEV_TYPE_SERVER)
        {
            //debug_msg("CLIENT INIT\n\r");
        }
    }
}