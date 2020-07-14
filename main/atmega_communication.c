#include "atmega_communication.h"


#define START_FRAME_WRITE 0xDE
#define START_FRAME_READ 0xFE
#define START_FRAME_CMD 0xEE

#define END_FRAME_WRITE 0xBE
#define END_FRAME_READ 0xFA
#define END_FRAME_CMD 0xEA

typedef enum
{
	AT_R_MEAS_ACCUM,
	AT_R_MEAS_MOTOR,
	AT_R_MEAS_SERVO,
	AT_R_LAST_POSITION,
}atmega_data_read_t;

typedef enum
{
	AT_W_MOTOR_VALUE,
	AT_W_SERVO_VALUE,
	AT_W_LAST_POSITION,
}atmega_data_write_t;

typedef enum
{
	AT_C_MOTOR_STATUS,
	AT_C_SERVO_STATUS,
	AT_C_LAST_POSITION,
}atmega_data_cmd_t;

#define at_send_data(data, len)

static uint32_t byte_received;
static uint8_t cmd, data_len;
static uint16_t data_write[AT_W_LAST_POSITION];
static uint16_t data_read[AT_R_LAST_POSITION];
static uint16_t data_cmd[AT_C_LAST_POSITION];

osTimerId vtId;
static void cbVtItvProto(void const *arg);
osTimerDef(VtItvProto, cbVtItvProto);

static void clear_msg(void) {
	byte_received = 0;
	cmd = 0;
	data_len = 0;
}

uint8_t send_buff[256];

void at_write_data(void) {
	uint8_t *pnt = data_write;

	send_buff[0] = START_FRAME_WRITE;
	send_buff[1] = sizeof(data_write);

	for (uint16_t i = 0; i < sizeof(data_write); i++) {
		send_buff[2 + i] = pnt[i];
	}
	at_send_data(send_buff, sizeof(data_write) + 2);
}

void at_read_byte(uint8_t byte) {
	if (byte_received == 0) {
		cmd = byte;
		byte_received++;
		/* Strat timer */
		return;
	}

	switch(cmd) {
		case START_FRAME_WRITE:
			if (byte_received == 1) {
				data_len = byte;
				byte_received++;
				if (data_len != sizeof(data_read)) {
					clear_msg();
				}
			}
			else if (byte_received - 2 < data_len) {
				uint8_t *pnt = (uint8_t *)data_read;
				pnt[byte_received - 2] = byte;
				byte_received++;
			}
			else {
				clear_msg();
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

static void cbVtItvProto(void const *arg)
{
	(void)arg;
	clear_msg();
}

void at_communication_init(void) {
	vtId = osTimerCreate(osTimer(VtItvProto), osTimerOnce, NULL);
}