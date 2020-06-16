#include "communication.h"
#include "parse_cmd.h"
#include "freertos/queue.h"

typedef struct 
{
	uint8_t type;
	parseCmd_t cmd;
	uint8_t len;
	uint8_t data[32];
}com_frame_t;

typedef struct 
{
	int (*keepAliveSend)(uint8_t * data, uint32_t dataLen);
}com_transport_t;


QueueHandle_t sendQueue;
