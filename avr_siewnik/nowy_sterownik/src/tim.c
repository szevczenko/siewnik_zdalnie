/*
 * tim.c
 *
 * Created: 23.01.2019 09:48:04
 *  Author: Demetriusz
 */ 
#include <config.h>
#if !TEST_APP
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#endif
#include <math.h>

#include "tim.h"
#include "config.h"

static uint8_t AutoReloadReg0 = 0;
static uint16_t prescaler0 = 0;
uint16_t counter = 0;
time_t mktime;
#if !TEST_APP
#if(F_CPU/TIM0_PRESCALER_VAL/TIM0_ARR < 1000)
#error "Nieprawidlowe wartosci konfiguracyjne tajmeru"
#endif
#if(F_CPU/TIM0_PRESCALER_VAL/TIM0_ARR > 1000000)
#error "Nieprawidlowe wartosci konfiguracyjne tajmeru"
#endif

ISR (TIMER0_COMP_vect)    // Timer1 ISR
{
	mktime.us += 50;//1000000/(F_CPU/prescaler0/AutoReloadReg0); //??????????
	if (mktime.us>=1000)
	{
		mktime.ms++;
		mktime.us -= 1000;
	} //us
}
#endif

void timer0_init(uint8_t prescaler, uint8_t ReloadReg)
{
	#if !TEST_APP
	AutoReloadReg0 = ReloadReg;
	TCCR0 |= (1<<WGM01);
	OCR0 = ReloadReg;
	switch(prescaler)
	{
		case PRE_1:
		TCCR0 |= (1<<CS00);
		prescaler0 = 1;
		break;
		case PRE_8:
		TCCR0 |= (1<<CS01);
		prescaler0 = 8;
		break;
		case PRE_64:
		TCCR0 |= (1<<CS00) | (1<<CS01);
		prescaler0 = 64;
		break;
		case PRE_256:
		TCCR0 |= (1<<CS02);
		prescaler0 = 256;
		break;
		case PRE_1024:
		TCCR0 |= (1<<CS02) | (1<<CS00);
		prescaler0 = 1024;
		break;
		default:
		TCCR0 |= (1<<CS10);
		break;
	}
	TIMSK |= (1 << OCIE0) ;   // Enable timer1 overflow interrupt(TOIE1)
	#endif
}

void evTime_init(evTime * evt)
{
	evt->time_state = TIMER_OFF;
	evt->timer = 0;
}

void evTime_start(evTime * evt, timer_t timeout)
{
	evt->time_state = TIMER_START;
	evt->timer = mktime.ms + timeout;
}

int evTime_is_stated(evTime * evt)
{
	return evt->time_state == TIMER_START;
}

timer_t evTime_time_to_end(evTime * evt)
{
	int time_to_end;
	if (evt->timer > mktime.ms)
		time_to_end = evt->timer - mktime.ms;
	else time_to_end = 0;
	return time_to_end;
}

int evTime_check(evTime * evt)
{
	if (evt->time_state == TIMER_START)
	{
		if (evt->timer < mktime.ms) 
		{
			evt->time_state = TIMER_END;
			return 1;
		}
		else return 0;
	}
	else return -1;
}

int evTime_off(evTime * evt)
{
	return evt->time_state == TIMER_OFF;
}

int evTime_process_period(evTime * timer, uint32_t period)
{
	static int ret_val;
	if (timer->time_state == TIMER_NO_INIT) evTime_init(timer);
	ret_val = evTime_check(timer);
	if (ret_val == 1)
	{
		evTime_start(timer, period);
		return 1;
	}
	else if (evTime_off(timer))
	{
		evTime_start(timer, period);
	}
	return 0;
}

#if TEST_APP
#include "stdio.h"
#include "unistd.h"
void test_mktime(void)
{
	usleep(1000);
	mktime.ms++;
}


static void process(evTime * evt)
{
	printf("Timer END. mktime: %d, timer %d\n",mktime.ms, evt->timer);
}

void test_evTime_process(void)
{
	static evTime test_timer;
	if (evTime_process_period(&test_timer, 1000))
	{
		process(&test_timer );
	}


}
#endif
