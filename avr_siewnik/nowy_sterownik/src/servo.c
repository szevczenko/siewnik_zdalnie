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
#include "servo.h"

#if CONFIG_DEVICE_SIEWNIK

sDriver servoD;

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

void servo_init(void)
{
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
	#endif
	servo_set_pwm_val((uint16_t)0);
	debug_msg("SERVO: init\n");
}


#endif //CONFIG_DEVICE_SIEWNIK








