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
#include "motor.h"
#include "vibro.h"
#include "esp_attr.h"


uint16_t test_value;
static gpio_config_t io_conf;
static uint32_t blink_pin = GPIO_NUM_2;

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

static void uart_init(uint32_t baud_rate) {
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    // We won't use a buffer for sending data.
    if (uart_driver_install(UART_NUM_0, 256, 256, 0, NULL, 0) != ESP_OK) {
        printf("UART: uart_driver_install error\n\r");
        return;
    }
    if (uart_param_config(UART_NUM_0, &uart_config) != ESP_OK) {
        printf("UART: uart_param_config error\n\r");
        return;
    }
}

void uartPrintfTimeout(const char *format, ...)
{
	static char buff[CONFIG_CONSOLE_VSNPRINTF_BUFF_SIZE];
	va_list ap;
	va_start(ap, format);
	int len = vsnprintf(buff, CONFIG_CONSOLE_VSNPRINTF_BUFF_SIZE, format, ap);
    va_end (ap);
    uart_write_bytes(UART_NUM_0, buff, len);
}

RTC_NOINIT_ATTR char * last_task_name;
static char * restart_task_name;

void debug_last_task(char * task_name) {
    last_task_name = task_name;
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

static void esp_task_wdt_isr(void * arg) {
    ets_printf("Wdt reset, last task: %s %p\n\r", last_task_name, last_task_name);
}

static uint32_t interruptMask;
static uint8_t interruptFlag;

void my_disable_interrupts(void) {
    if (interruptFlag == 0) {
        __asm__ volatile ("rsil %0, " XTSTR(XCHAL_EXCM_LEVEL) : "=a" (cpu_sr) :: "memory");
    }
    else {
        uint32_t mask = soc_get_int_mask();
        interruptMask = mask;
        for (int i = 0; i < ETS_INT_MAX && mask; i++) {
            int bit = 1 << i;
            if (!(bit & mask) || i == ETS_WDT_INUM)
                continue;
            _xt_isr_mask(1 << i);
        }
    }
}

void my_enable_interrupts(void) {
    if (interruptFlag == 0) {
        __asm__ volatile ("wsr %0, ps" :: "a" (cpu_sr) : "memory");
    }
    else {
        uint32_t mask = interruptMask;
        for (int i = 0; i < ETS_INT_MAX && mask; i++) {
            int bit = 1 << i;
            if (!(bit & mask) || i == ETS_WDT_INUM)
                continue;
            _xt_isr_unmask(1 << i);
        }
    }
}

int __esp_os_init(void) {
    restart_task_name = last_task_name;
    return 0;
}

void app_main()
{
    configInit();
    checkDevType();

    wifiDrvInit();
    keepAliveStartTask();
    menuParamInit();

    if (config.dev_type != T_DEV_TYPE_SERVER)
    {
        uart_init(CONFIG_CONSOLE_SERIAL_SPEED);
        battery_init();
        init_buttons();
        fastProcessStartTask();
        ssd1306_Init();
        init_menu();
        blink_pin = GPIO_NUM_15;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (1 << GPIO_NUM_15) | (1 << GPIO_NUM_2) | (1 << blink_pin);
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;
        gpio_config(&io_conf);
    }
    else {
        uart_init(57600);
        vibro_init();
        at_communication_init();
        motor_init();
        //errorSiewnikStart();
        measure_start();
        //LED on
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (1 << blink_pin);
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;
        gpio_config(&io_conf);
    }

    _xt_isr_attach(ETS_WDT_INUM, esp_task_wdt_isr, NULL);
    ets_printf("Init last task: %s %p\n\r", restart_task_name, restart_task_name);

    while(1)
    {
        vTaskDelay(MS2ST(975));
        if (config.dev_type == T_DEV_TYPE_SERVER)
        {
           gpio_set_level(blink_pin, 0);
        }
        
        vTaskDelay(MS2ST(25));

        if (config.dev_type == T_DEV_TYPE_SERVER)
        {
           gpio_set_level(blink_pin, 1);
        }
        if (config.dev_type != T_DEV_TYPE_SERVER)
        {
            //debug_msg("CLIENT INIT\n\r");
        }
    }
}