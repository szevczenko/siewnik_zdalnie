/*
 * pwm.c
 *
 * Created: 07.02.2019 13:19:09
 *  Author: Demetriusz
 */ 
#include <config.h>
#include <stdio.h>
#if !TEST_APP
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#endif //!TEST_APP

#include "tim.h"
//#include "disp.h"
#include "servo.h"
#if CONFIG_DEVICE_SIEWNIK

static void servo_exit_try(void);

sDriver servoD;
static uint8_t try_count = 0;

static void set_pwm(uint16_t pwm)
{
	#if !TEST_APP
	OCR1A = pwm;
	#endif
	//OCR1B = pwm;
}

void servo_set_pwm_val(uint16_t value)
{
	//debug_msg("REG: close %d, open %d, pwm %d\n", min, max, pwm);
	set_pwm(value);
}

void servo_init(uint8_t prescaler)
{
	(void) prescaler;
	#if !TEST_APP
	ICR1 = 19999;
	DDRD |= (1 << 4) | (1<<5); //?????????? TO DO
	// set TOP to 16bit
	OCR1B = 0x0;
	OCR1A = 0x0;
	TCCR1A |= (1 << WGM11);
	TCCR1B |= (1 << WGM12)|(1 << WGM13);
	TCCR1A |=  (1<<COM1A1); //(1 << COM1B1) |
	TCCR1B |= (1<<CS11);
	set_pwm(19999);
	LED_SERVO_OFF;
	#endif
	servo_set_pwm_val((uint16_t)0);
	servoD.state = SERVO_CLOSE;
	servoD.value = 0;
	evTime_init(&servoD.timeout);
	servoD.try_cnt = 0;
	try_count = 0;
	debug_msg("SERVO: init\n");
}

int servo_is_open(void)
{
	return servoD.state == SERVO_OPEN;
}

int servo_open(uint16_t value) // value - 0-100%
{
	if (servoD.state == SERVO_CLOSE || servoD.state == SERVO_OPEN)
	{
		servoD.state = SERVO_OPEN;
		servoD.value = value;
		servo_set_pwm_val((uint16_t)value);
		//debug_msg("SERVO_OPPENED %d\n", value);
		LED_SERVO_ON;
		return 1;
	}
	
	return 0;
}

int servo_close(void)
{
	if (servo_is_open())
	{
		servo_set_pwm_val((uint16_t)0);
		servoD.state = SERVO_CLOSE;
		servoD.value = 0;
		debug_msg("SERVO_CLOSED %d\n", servoD.value);
		LED_SERVO_OFF;
		return 1;
	}

	return 0;
}

void servo_process(uint16_t value)
{
	static evTime servo_timer;
	if (evTime_process_period(&servo_timer, 75))
	{
		servo_set_pwm_val((uint16_t)value);
	}
}

#endif //CONFIG_DEVICE_SIEWNIK








