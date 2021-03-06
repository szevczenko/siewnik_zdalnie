#ifndef _PARSE_CMD_H
#define _PARSE_CMD_H
#include "config.h"

#define CMD_REQEST 0x11
#define CMD_ANSWER 0x22
#define CMD_DATA 0x33
#define CMD_COMMAND 0x44

#define POSITIVE_RESP 0xFF
#define NEGATIVE_RESP 0xFE

typedef enum
{
	PC_KEEP_ALIVE,
	PC_SET,
	PC_GET,
	PC_SET_ALL,
	PC_GET_ALL,
	PC_LAST
}parseType_t;

typedef enum
{
	PC_CMD_NONE,
	PC_CMD_RESET_ERROR,
	PC_CMD_LAST
}parseCmd_t;

typedef enum
{
	MENU_MOTOR,
	MENU_SERVO,
	MENU_VIBRO_PERIOD,
	MENU_VIBRO_WORKING_TIME,
	MENU_MOTOR_IS_ON,
	MENU_SERVO_IS_ON,
	MENU_MOTOR_ERROR_IS_ON,
	MENU_SERVO_ERROR_IS_ON,
	MENU_CURRENT_SERVO,
	MENU_CURRENT_MOTOR,
	MENU_VOLTAGE_ACCUM,
	MENU_TEMPERATURE,
	MENU_ERRORS,
	MENU_START_SYSTEM,

	/* calibration value */
	MENU_ERROR_SERVO,
	MENU_ERROR_MOTOR,
	MENU_ERROR_SERVO_CALIBRATION,
	MENU_ERROR_MOTOR_CALIBRATION,
	MENU_MOTOR_MIN_CALIBRATION,
	MENU_MOTOR_MAX_CALIBRATION,
	MENU_BUZZER,
	MENU_CLOSE_SERVO_REGULATION,
	MENU_OPEN_SERVO_REGULATION,
	MENU_TRY_OPEN_CALIBRATION,
	MENU_LAST_VALUE

}menuValue_t;

typedef struct  
{
	uint16_t max_value;
	uint16_t default_value;
}menuPStruct_t;

void parse_client(uint8_t * buff, uint32_t len);
void parse_server(uint8_t * buff, uint32_t len);

void parse_client_buffer(uint8_t * buff, uint32_t len);
void parse_server_buffer(uint8_t * buff, uint32_t len);

#endif