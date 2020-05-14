#ifndef _FAST_ADD_H
#define _FAST_ADD_H

#include "config.h"

typedef enum
{
	FP_MINUS,
	FP_PLUS
}fast_process_sign;

typedef struct 
{
	uint32_t counter;
	uint32_t delay;
	uint8_t * value;
	uint8_t max, min, sign;
	void (*func)(uint8_t);
}fast_add_t;

void fastProcessStart(uint8_t * value, uint8_t max, uint8_t min, fast_process_sign sign, void (*func)(uint8_t));
void fastProcessStop(uint8_t * value);
void fastProcessDeInit(void);
void fastProcessStartTask(void);

#endif