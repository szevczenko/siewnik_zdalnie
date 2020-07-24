/*
dcmotorpwm lib 0x01

copyright (c) Davide Gironi, 2012

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#include <stdio.h>

#include "config.h"

#if !TEST_APP
#include <avr/io.h>
#include <avr/interrupt.h>
#endif

#include "dcmotorpwm.h"

mDriver motorD;
/*
 * init a motor
 */
void dcmotorpwm_init(void) {
	debug_msg("dcmotor init\n");
	evTime_init(&motorD.timeout);
	motorD.state = MOTOR_OFF;
	LED_MOTOR_OFF;
	TCCR2 = 0;
	DCMOTORPWM_DDR |= (1<<DCMOTORPWM_PIN1);
	#if CONFIG_DEVICE_SOLARKA
	CLEAR_PIN(DCMOTORPWM_PORT, DCMOTORPWM_PIN1);
	#else
	SET_PIN(DCMOTORPWM_PORT, DCMOTORPWM_PIN1);
	#endif
}

void dcmotorpwm_deinit(void)
{
	//debug_msg("dcmotor deinit\n");
	motorD.state = MOTOR_NO_INIT;
	#if !TEST_APP
	TCCR2 = 0;
	#if CONFIG_DEVICE_SOLARKA
	CLEAR_PIN(DCMOTORPWM_PORT, DCMOTORPWM_PIN1);
	#else
	SET_PIN(DCMOTORPWM_PORT, DCMOTORPWM_PIN1);
	#endif
	LED_MOTOR_OFF;
	#endif
}

/*
 * stop the motor
 */
int dcmotorpwm_stop(void) {
	
	debug_msg("dcmotor stop\n");
	TCCR2 = 0;
	#if CONFIG_DEVICE_SOLARKA
	CLEAR_PIN(DCMOTORPWM_PORT, DCMOTORPWM_PIN1);
	#else
	SET_PIN(DCMOTORPWM_PORT, DCMOTORPWM_PIN1);
	#endif
	//OCR2 = 0;
	motorD.last_state = motorD.state;
	motorD.state = MOTOR_OFF;
	return 1;
}

int dcmotorpwm_start(void)
{
	if (motorD.state == MOTOR_OFF)
	{
		//debug_msg("Motor Start\n")
		
		#if CONFIG_DEVICE_SOLARKA
		TCCR2 |= (1<<COM21); 
		#else
		TCCR2 |= (1<<COM21) | (1<<COM20); 
		#endif
		
		TCCR2 |= (1<<WGM20);
		TCCR2 |= DCMOTORPWM_PRESCALER; //set prescaler
		motorD.last_state = motorD.state;
		motorD.state = MOTOR_AXELERATE;
		evTime_start(&motorD.timeout, 1000);
		return 1;
	}
	else 
	{
		//debug_msg("dcmotor canot start\n");
		return 0;
	}
}

void dcmotor_process(uint16_t value)
{
	static evTime dcmotor_timer;
	if (evTime_process_period(&dcmotor_timer, 150))
	{
		//debug_msg("Process %d\n", motorD.state);
		switch(motorD.state)
		{
			case MOTOR_ON:
			OCR2 = value;
			break;

			case MOTOR_OFF:
			break;

			case MOTOR_AXELERATE:
			motorD.state = MOTOR_ON; //!!
			break;
		}
		
	}
	#if !TEST_APP
	
	#endif
}


