/*
 * pwm.h
 *
 * Created: 07.02.2019 13:19:21
 *  Author: Demetriusz
 */ 


#ifndef PWM_H_
#define PWM_H_
#include "tim.h"
#include "config.h"

//#define SERVO_PORT DDRD
//#define SERVO_PIN 

#if CONFIG_DEVICE_SIEWNIK

#define OFF_SERVO servo_set_pwm_val(19999);

typedef struct  
{
	uint16_t value; // 0 - 100%
}sDriver;

void servo_init(void);
void servo_set_pwm_val(uint16_t value);

#endif



#endif /* PWM_H_ */