#include "atmega_communication.h"
#include "menu_param.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#include "freertos/timers.h"

#include "motor.h"
#include "servo.h"
#include "vibro.h"



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

static void read_uart_data(void * arg)
{
	char input;
	while (1)
	{
		if (uart_read_bytes(UART_NUM_0, (uint8_t *)&input, 1, 100) > 0) {
			at_read_byte((uint8_t) input);
		}
	}
}

static void atm_com(void * arg) {
	uint16_t motor_pmw, servo_pwm;
	while(1) {
		vTaskDelay(150 / portTICK_RATE_MS);
		taskENTER_CRITICAL();
		motor_pmw = dcmotor_process((uint8_t)menuGetValue(MENU_MOTOR));
		data_write[AT_W_MOTOR_VALUE] = motor_pmw;
		#if CONFIG_DEVICE_SIEWNIK
		servo_pwm = servo_process((uint8_t)menuGetValue(MENU_SERVO));
		data_write[AT_W_SERVO_VALUE] = (uint16_t)servo_pwm;
		data_write[AT_W_SERVO_IS_ON] = (uint16_t)menuGetValue(MENU_SERVO_IS_ON);
		#endif

		#if CONFIG_DEVICE_SOLARKA
		vibro_config(menuGetValue(MENU_VIBRO_PERIOD) * 1000, menuGetValue(MENU_VIBRO_WORKING_TIME) * 1000);
		if (menuGetValue(MENU_SERVO_IS_ON)) {
			vibro_start();
		}
		else {
			vibro_stop();
		}
		data_write[AT_W_SERVO_IS_ON] = (uint16_t)vibro_is_on();
		#endif
		
		data_write[AT_W_MOTOR_IS_ON] = (uint16_t)menuGetValue(MENU_MOTOR_IS_ON);

		if (data_write[AT_W_MOTOR_IS_ON]) {
			motor_start();
		}
		else {
			motor_stop();
		}

		at_write_data();
		taskEXIT_CRITICAL();
		//debug_msg("mot: %d %d, servo: %d %d \n\r", data_write[AT_W_MOTOR_IS_ON], data_write[AT_W_MOTOR_VALUE], data_write[AT_W_SERVO_IS_ON], data_write[AT_W_SERVO_VALUE]);
	}
}

void at_communication_init(void) {
	xTaskCreate(atm_com, "atm_com", 1024, NULL, 10, NULL);
	xTaskCreate(read_uart_data, "read_uart_data", 1024, NULL, 10, NULL);
	xTimers = xTimerCreate("at_com_tim", 100 / portTICK_RATE_MS, pdFALSE, ( void * ) 0, vTimerCallback);
}