/*
dcmotorpwm lib 0x01

copyright (c) Davide Gironi, 2012

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#ifndef DCMOTORPWM_H
#define DCMOTORPWM_H
#include "tim.h"
#include "stdint.h"
#include "config.h"
//set motor port
#define DCMOTORPWM_DDR DDRD
#define DCMOTORPWM_PORT PORTD
#define DCMOTORPWM_PIN1 7
//#define DCMOTORPWM_PIN2 PB2

//set minimum velocity
#if CONFIG_DEVICE_SOLARKA
#define DCMOTORPWM_MINVEL 200
#endif

#if CONFIG_DEVICE_SIEWNIK
#define DCMOTORPWM_MINVEL dark_menu_get_value(MENU_MOTOR_MINIMUM_REGULATION)
#endif

//freq = 1 / time
//pulse freq = FCPU / prescaler * ICR1
// 125 = 1000000 / (8 * 1000)
#define DCMOTORPWM_ICR2 255

#if CONFIG_DEVICE_SIEWNIK
#define DCMOTORPWM_PRESCALER (1 << CS20)
#endif

#if CONFIG_DEVICE_SOLARKA
#define DCMOTORPWM_PRESCALER (1 << CS21)
#endif


typedef enum
{
	MOTOR_NO_INIT = 0,
	MOTOR_OFF,
	MOTOR_ON,
	MOTOR_TRY,
	MOTOR_AXELERATE,
	MOTOR_ERROR ,
	MOTOR_REGULATION,
}motorState;

typedef struct  
{
	uint8_t state;
	uint8_t last_state;
	uint8_t error_code;
	uint8_t pwm_value;
	evTime timeout;
	uint8_t try_cnt;
	
}mDriver;

//functions
extern void dcmotorpwm_init(void);
void dcmotorpwm_deinit(void);
extern int dcmotorpwm_stop(void);
extern int dcmotorpwm_start(void);
extern int dcmotor_is_on(void);
void dcmotor_process(uint16_t value);
void dcmotor_set_error(void);
int dcmotor_set_try(void);
int dcmotor_set_normal_state(void);
int dcmotor_get_pwm(void);
void dcmotorpwm_goforward(uint8_t vel);
void dcmotorpwm_gobackward(uint8_t vel);
void motor_regulation(uint8_t pwm);

#endif
