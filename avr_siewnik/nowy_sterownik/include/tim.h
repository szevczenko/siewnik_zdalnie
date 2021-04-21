/*
 * tim.h
 *
 * Created: 23.01.2019 09:48:18
 *  Author: Demetriusz
 */ 


#ifndef TIM_H_
#define TIM_H_

#include <stdint.h>
#include "config.h"

void timer_init(uint8_t prescaler, uint16_t ReloadReg);
void timer0_init(uint8_t prescaler, uint8_t ReloadReg);
typedef uint32_t timer_t;

typedef enum
{
	PRE_1,
	PRE_8,
	PRE_64,
	PRE_256,
	PRE_1024,
}   prescaler_t;

typedef volatile struct
{
	uint16_t us;
	uint32_t ms;
}time_t;

typedef enum
{
	TIMER_NO_INIT = 0,
	TIMER_START,
	TIMER_END,
	TIMER_OFF,
}timeState;

typedef struct
{
	timer_t timer;
	uint8_t time_state;
}evTime;
	
extern time_t mktime;

void evTime_init(evTime * evt);
void evTime_start(evTime * evt, timer_t timeout);
int evTime_off(evTime * evt);
int evTime_check(evTime * evt);
int evTime_is_stated(evTime * evt);
int evTime_process_period(evTime * timer, uint32_t period);
timer_t evTime_time_to_end(evTime * evt);

#if TEST_APP
void test_mktime(void);
void test_evTime_process(void);
#endif


#endif /* TIM_H_ */