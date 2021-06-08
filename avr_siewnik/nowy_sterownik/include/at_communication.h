#ifndef ATMEGA_COMMUNICATION_H
#define ATMEGA_COMMUNICATION_H
#include "config.h"

#define VIBRO_PORT PORTD
#define VIBRO_DDR DDRD
#define VIBRO_PIN 4

#define SYSTEM_PORT PORTB
#define SYSTEM_DDR DDRB
#define SYSTEM_PIN 4

#define VIBRO_INIT_PIN	SET_PIN(VIBRO_DDR, VIBRO_PIN)
#define ON_VIBRO_PIN	SET_PIN(VIBRO_PORT, VIBRO_PIN)
#define OFF_VIBRO_PIN	CLEAR_PIN(VIBRO_PORT, VIBRO_PIN)

#define SYSTEM_INIT_PIN	SET_PIN(SYSTEM_DDR, SYSTEM_PIN)
#define ON_SYSTEM_PIN	SET_PIN(SYSTEM_PORT, SYSTEM_PIN)
#define OFF_SYSTEM_PIN	CLEAR_PIN(SYSTEM_PORT, SYSTEM_PIN)

#define START_FRAME_WRITE 0xDE
#define START_FRAME_READ 0xFE
#define START_FRAME_CMD 0xEE

#define END_FRAME_WRITE 0xBE
#define END_FRAME_READ 0xFA
#define END_FRAME_CMD 0xEA

typedef enum
{
	AT_W_MEAS_ACCUM,
	AT_W_MEAS_MOTOR,
	AT_W_MEAS_SERVO,
	AT_W_LAST_POSITION,
}atmega_data_write_t;

typedef enum
{
	AT_R_MOTOR_VALUE,
	AT_R_SERVO_VIBRO_VALUE,
	AT_R_MOTOR_IS_ON,
	AT_R_SERVO_VIBRO_IS_ON,
	AT_R_SYSTEM_ON,
	AT_R_LAST_POSITION,
}atmega_data_read_t;

typedef enum
{
	AT_C_MOTOR_STATUS,
	AT_C_SERVO_STATUS,
	AT_C_LAST_POSITION,
}atmega_data_cmd_t;

void at_communication_init(void);
void at_read_byte(uint8_t byte);
uint16_t atmega_get_data(atmega_data_read_t data_type);
void at_read_data_process(void);
void atm_com_process(void);

#endif