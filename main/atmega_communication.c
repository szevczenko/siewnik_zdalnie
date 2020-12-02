#include "atmega_communication.h"
#include "menu_param.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "console.h"
#include "freertos/timers.h"
#include "console.h"
#include "motor.h"
#include "servo.h"



#define at_send_data(data, len) uart_write_bytes(UART_NUM_0, (char *)data, len)

static uint32_t byte_received;
static uint8_t cmd, data_len;
static uint16_t data_write[AT_W_LAST_POSITION];
static uint16_t data_read[AT_R_LAST_POSITION];
//static uint16_t data_cmd[AT_C_LAST_POSITION];

static TimerHandle_t xTimers;

static void clear_msg(void) {
	byte_received = 0;
	cmd = 0;
	data_len = 0;
	xTimerStop(xTimers, 0);
}

static void vTimerCallback( TimerHandle_t xTimer )
{
	(void)xTimer;
	clear_msg();
	//debug_msg("vTimerCallback\n\r");
}

uint8_t send_buff[256];

void at_write_data(void) {
	uint8_t *pnt = (uint8_t *)data_write;

	send_buff[0] = START_FRAME_WRITE;
	send_buff[1] = sizeof(data_write);

	for (uint16_t i = 0; i < sizeof(data_write); i++) {
		send_buff[2 + i] = pnt[i];
	}
	at_send_data(send_buff, sizeof(data_write) + 2);
}

uint16_t atmega_get_data(atmega_data_read_t data_type) {
	if (data_type < AT_R_LAST_POSITION) {
		return data_read[data_type];
	}
	return 0;
}

static void atmega_set_read_data(void) {
	
}

void at_read_byte(uint8_t byte) {
	if (byte_received == 0) {
		if (byte == START_FRAME_WRITE || byte == START_FRAME_READ || byte == START_FRAME_CMD) {
			cmd = byte;
			byte_received++;
			xTimerStart( xTimers, 0 );
			return;
		}
		else {
			/* Debug logs */
			debug_data(&byte, 1);
		}
	}

	switch(cmd) {
		case START_FRAME_WRITE:
			if (byte_received == 1) {
				data_len = byte;
				byte_received++;
				if (data_len != sizeof(data_read)) {
					clear_msg();
					debug_msg("FRAME BAD LEN\n\r");
				}
				xTimerStart( xTimers, 0 );
			}
			else if (byte_received - 2 < data_len) {
				uint8_t *pnt = (uint8_t *)data_read;
				pnt[byte_received - 2] = byte;
				byte_received++;
				if (byte_received - 2 == data_len) {
					/* Verify data */
					clear_msg();
				}
				xTimerStart( xTimers, 0 );
			}
			else {
				clear_msg();
				//debug_msg("ATMEGA RECEIVE UNKNOW ERROR\n\r");
			}
			break;

		case START_FRAME_READ:
			/* SEND BUFF data_write */
			at_write_data();
			clear_msg();
			break;

		case START_FRAME_CMD:
			/* Nothing for host */
			clear_msg();
			break;

		default:
			clear_msg();
	}
}

static void atm_com(void * arg) {
	
	while(1) {
		taskENTER_CRITICAL();
		data_write[AT_W_MOTOR_VALUE] = (uint16_t)dcmotor_process((uint8_t)menuGetValue(MENU_MOTOR));
		data_write[AT_W_SERVO_VALUE] = (uint16_t)servo_process((uint8_t)menuGetValue(MENU_SERVO));
		data_write[AT_W_MOTOR_IS_ON] = (uint16_t)menuGetValue(MENU_MOTOR_IS_ON);
		data_write[AT_W_SERVO_IS_ON] = (uint16_t)menuGetValue(MENU_SERVO_IS_ON);
		at_write_data();
		taskEXIT_CRITICAL();
		vTaskDelay(200 / portTICK_RATE_MS);
	}
}

// static void uart_init(uint32_t baud_rate) {
//     uart_config_t uart_config = {
//         .baud_rate = baud_rate,
//         .data_bits = UART_DATA_8_BITS,
//         .parity = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//     };
//     // We won't use a buffer for sending data.
//     if (uart_driver_install(CONFIG_CONSOLE_SERIAL, 256, 256, 0, NULL, 0) != ESP_OK) {
//         printf("UART: uart_driver_install error\n\r");
//         return;
//     }
//     if (uart_param_config(CONFIG_CONSOLE_SERIAL, &uart_config) != ESP_OK) {
//         printf("UART: uart_param_config error\n\r");
//         return;
//     }
// 	serial_init_flag = 1;
// }

void at_communication_init(void) {
	xTaskCreate(atm_com, "atm_com", 1024, NULL, 10, NULL);
	con0serial.console_mode = CON_MODE_AT_COM;
	xTimers = xTimerCreate("at_com_tim", 100 / portTICK_RATE_MS, pdFALSE, ( void * ) 0, vTimerCallback);
}