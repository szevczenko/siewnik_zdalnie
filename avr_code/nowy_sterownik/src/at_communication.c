#include "at_communication.h"
#include "measure.h"
#include "config.h"
#include "tim.h"
#include "accumulator.h"
#include "usart.h"
#include "dcmotorpwm.h"
#include "servo.h"

void at_send_data(char *data, int len)  {
	for (int i = 0; i < len; i++) {
		uart_putc(data[i]);
	}
}

static uint32_t byte_received;
static uint8_t cmd, data_len;
static uint16_t data_write[AT_W_LAST_POSITION];
static uint16_t data_read[AT_R_LAST_POSITION];
//static uint16_t data_cmd[AT_C_LAST_POSITION];

static evTime xTimers;

static void clear_msg(void) {
	byte_received = 0;
	cmd = 0;
	data_len = 0;
	evTime_off(&xTimers);
}

static void vTimerCallback( evTime *xTimer )
{
	(void)xTimer;
	clear_msg();
	debug_msg("vTimerCallback\n\r");
}

uint8_t send_buff[256];

void at_write_data(void) {
	uint8_t *pnt = (uint8_t *)data_write;

	send_buff[0] = START_FRAME_WRITE;
	send_buff[1] = sizeof(data_write);

	for (uint16_t i = 0; i < sizeof(data_write); i++) {
		send_buff[2 + i] = pnt[i];
	}
	at_send_data((char *)send_buff, sizeof(data_write) + 2);
}

uint16_t atmega_get_data(atmega_data_read_t data_type) {
	if (data_type < AT_R_LAST_POSITION) {
		return data_read[data_type];
	}
	return 0;
}

static void atmega_set_read_data(void) {
	
}

void at_read_data_process(void) {
	static uint32_t atm_timer;
	if (atm_timer < mktime.ms) {
		
		uint8_t byte, error_val;
		uint16_t data = uart_getc();
		
		error_val = data & 0xFF00;
		byte = data & 0x00FF;
		if (error_val == UART_NO_DATA) {
			atm_timer = mktime.ms + 50;
			return;
		}
		
		at_read_byte(byte);
		
		if (evTime_check(&xTimers)) {
			vTimerCallback(&xTimers);
		}
		atm_timer = mktime.ms + 5;
		
	}
}

uint16_t motor_read_value;
uint16_t servo_read_value;
uint16_t servo_vibro_is_on;
uint16_t motor_is_on;

void data_process(void) {
	motor_read_value = atmega_get_data(AT_R_MOTOR_VALUE) & 0xFF;
	servo_read_value = atmega_get_data(AT_R_SERVO_VIBRO_VALUE);
	servo_vibro_is_on = atmega_get_data(AT_R_SERVO_VIBRO_IS_ON);
	motor_is_on = atmega_get_data(AT_R_MOTOR_IS_ON);
	
	debug_msg("mot: %d %d, servo: %d %d \n\r",motor_is_on, motor_read_value, servo_vibro_is_on, servo_read_value);

	/* MOTOR SECTION */
	if (motor_is_on) {
		dcmotorpwm_start();
		dcmotor_set_pwm((uint8_t) motor_read_value);
	}
	else {
		dcmotorpwm_stop();
	}
	
	/* SERVO SECTION */
	/*if (servo_vibro_is_on) {
		servo_open(servo_read_value);
	}
	else {
		servo_close();
	}*/

	/* VIBRO SECTION */
	if (servo_vibro_is_on) {
		ON_VIBRO_PIN;
	}
	else {
		OFF_VIBRO_PIN;
	}
}

void at_read_byte(uint8_t byte) {
	if (byte_received == 0) {
		cmd = byte;
		byte_received++;
		evTime_start(&xTimers, 200);
		return;
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
			evTime_start(&xTimers, 200);
		}
		else if (byte_received - 2 < data_len) {
			uint8_t *pnt = (uint8_t *)data_read;
			pnt[byte_received - 2] = byte;
			byte_received++;
			if (byte_received - 2 == data_len) {
				clear_msg();
				/* Verify data */
				data_process();
				
			}
			evTime_start(&xTimers, 200);
		}
		else {
			/* End receive data or unknown error */
			clear_msg();
			debug_msg("ATMEGA RECEIVE UNKNOW ERROR\n\r");
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
		//debug_msg("FRAME BAD START\n\r");
		clear_msg();
	}
}

void atm_com_process(void) {
	static uint32_t atm_timer;
	if (atm_timer < mktime.ms)
	{
		/* Do poprawy */
		data_write[AT_W_MEAS_ACCUM] = 123;//(uint16_t)accum_get_voltage();
		data_write[AT_W_MEAS_MOTOR] = 321;//(uint16_t)measure_get_current(MEAS_MOTOR, MOTOR_RESISTOR);
		data_write[AT_W_MEAS_SERVO] = 4095;//(uint16_t)measure_get_filtered_value(MEAS_SERVO);
		at_write_data();
		atm_timer = mktime.ms + 200;
	}
}


void at_communication_init(void) {
	VIBRO_INIT_PIN;
	evTime_init(&xTimers);
}