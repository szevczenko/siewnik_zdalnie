#include <stdio.h>

#include "config.h"

#include "motor.h"

#define debug_msg(...)
#define LED_MOTOR_OFF
#define LED_MOTOR_ON
#define CMD_MOTOR_OFF
#define CMD_MOTOR_ON
#define CMD_MOTOTR_SET_PWM(pwm)

mDriver motorD;
/*
 * init a motor
 */
void motor_init(void) {
	debug_msg("dcmotor init\n");
	motorD.state = MOTOR_OFF;
	LED_MOTOR_OFF;
	CMD_MOTOR_OFF;
}

void motor_deinit(void)
{
	//debug_msg("dcmotor deinit\n");
	motorD.state = MOTOR_NO_INIT;
	CMD_MOTOR_OFF;
	LED_MOTOR_OFF;
}

/*
 * stop the motor
 */
int motor_stop(void) {
	
	//set orc
	if (dcmotor_is_on())
	{
		debug_msg("dcmotor stop\n");
		CMD_MOTOR_OFF;
		LED_MOTOR_OFF;
		motorD.last_state = motorD.state;
		motorD.state = MOTOR_OFF;
		return 1;
	}
	else
	{
		 //debug_msg("dcmotor cannot stop\n");
	}
	return 0;
}

int dcmotor_is_on(void)
{
	if (motorD.state == MOTOR_ON || motorD.state == MOTOR_AXELERATE || motorD.state == MOTOR_TRY)
	{
		return 1;
	} 
	else return 0;
}

int motor_start(void)
{
	if (motorD.state == MOTOR_OFF)
	{
		//debug_msg("Motor Start\n");
		LED_MOTOR_ON;
		CMD_MOTOR_ON;
		motorD.last_state = motorD.state;
		motorD.state = MOTOR_AXELERATE;
		motorD.timeout = xTaskGetTickCount() + MS2ST(1000);
		return 1;
	}
	else 
	{
		//debug_msg("dcmotor canot start\n");
		return 0;
	}
}

static uint8_t count_pwm(int pwm)
{
	return /* DCMOTORPWM_MINVEL + (156 +  dark_menu_get_value(MENU_MOTOR_MAXIMUM_REGULATION)  - DCMOTORPWM_MINVEL)* */ pwm/99;
}

int dcmotor_set_pwm(int pwm)
{
	if (pwm >= 0 && pwm < 100)
	{
		//debug_msg("dcmotor_set_pwm %d\n", pwm);
		motorD.pwm_value = pwm;
		CMD_MOTOTR_SET_PWM(count_pwm(motorD.pwm_value));
		return 1;
	}
	else
	{
		return 0;
	}
}

int dcmotor_get_pwm(void)
{
	return motorD.pwm_value;
}

void dcmotor_set_error(void)
{
	debug_msg("dcmotor error\n");
	motor_stop();
	motorD.state = MOTOR_ERROR;
}

int dcmotor_set_try(void)
{
	if (dcmotor_is_on())
	{
		motorD.state = MOTOR_TRY;
		return 1;
	}
	return 0;
}

int dcmotor_set_normal_state(void)
{
	if (dcmotor_is_on())
	{
		motorD.state = MOTOR_ON;
		return 1;
	}
	return 0;
}

void motor_regulation(uint8_t pwm) {
	motorD.state = MOTOR_REGULATION;
	motorD.pwm_value = pwm;
}

uint8_t dcmotor_process(uint8_t value)
{
	switch(motorD.state)
	{
		case MOTOR_ON:
		dcmotor_set_pwm(value);
		break;

		case MOTOR_OFF:
		motorD.pwm_value = 0;
		break;

		case MOTOR_TRY:
			if (value <= 50)
			{
				dcmotor_set_pwm(value + 20);
			}
			else if ((value > 50) && (value <= 70))
			{
				dcmotor_set_pwm(value + 15);
			}
			else
			{
				dcmotor_set_pwm(value);
			}
		break;

		case MOTOR_ERROR:
			CMD_MOTOR_OFF;
		break;

		case MOTOR_AXELERATE:
			motorD.state = MOTOR_ON; //!!
			break;					 //!
			dcmotor_set_pwm(50);
			
			//debug_msg("MOTOR_AXELERATE %d\n", motorD.pwm_value);
			if (motorD.timeout < xTaskGetTickCount()) {
				motorD.state = MOTOR_ON;
			}

		break;
			
		case MOTOR_REGULATION:
			dcmotor_set_pwm(value);
		break;

	}
		
	return motorD.pwm_value;
}

