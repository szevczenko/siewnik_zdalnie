/*
 * IncFile1.h
 *
 * Created: 23.01.2019 09:50:46
 *  Author: Demetriusz
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>
#include <avr/io.h>
#include "tim.h"

#define MOTOR_RESISTOR 0.033

#define TRUE 1
#define FALSE 0 
//////////////////////////////////////////////////
// DEVICE
#define CONFIG_DEVICE_SOLARKA 1
#define CONFIG_DEVICE_SIEWNIK 0

#if CONFIG_DEVICE_SOLARKA
#define CONFIG_VIBRO_TIME_ON 1
#endif

//////////////////////////////////////////////////
// DEVICE ERROR
#if (CONFIG_DEVICE_SOLARKA && CONFIG_DEVICE_SIEWNIK) || (!CONFIG_DEVICE_SOLARKA && !CONFIG_DEVICE_SIEWNIK)
#error "Wlacz tylko 1 urrzadzenie, solarka albo siewnik";
#endif

#define F_CPU 8000000L

#define TIM0_PRESCALER PRE_8
#define TIM0_PRESCALER_VAL 8
#define TIM0_ARR 50

#define BUZZER_ON 1

#define LED_DISP_CNT 4
#define COUNTER_MAX 40  //Od tego zalezy chestotliwosc i wypelnienie tajmerow F_CPU/TIM_PRESCALER/TIME_ARR/COUNTER_MAX/2 = 50[Hz]

#define SET_PIN(port, pin) port |= (1<<pin)
#define CLEAR_PIN(port,pin) port &= ~(1<<pin)
#define TOGGLE_PIN(port, pin) port ^=(1<<pin)

#define LED_MOTOR_PORT PORTD
#define LED_MOTOR_PIN 4
#define LED_MOTOR_DDR DDRD

#define LED_SERVO_PORT PORTD
#define LED_SERVO_PIN 6
#define LED_SERVO_DDR DDRD

#define LED_ACUM_PORT PORTB
#define LED_ACUM_PIN 5
#define LED_ACUM_DDR DDRB

#define LED_MOTOR_ON SET_PIN(LED_MOTOR_PORT, LED_MOTOR_PIN)
#define LED_MOTOR_OFF CLEAR_PIN(LED_MOTOR_PORT, LED_MOTOR_PIN)
#define LED_SERVO_ON SET_PIN(LED_SERVO_PORT, LED_SERVO_PIN)
#define LED_SERVO_OFF CLEAR_PIN(LED_SERVO_PORT, LED_SERVO_PIN)
#define LED_SERVO_TOGGLE TOGGLE_PIN(LED_SERVO_PORT, LED_SERVO_PIN)
#define LED_MOTOR_TOGGLE TOGGLE_PIN(LED_MOTOR_PORT, LED_MOTOR_PIN)

#define LED_ACUM_OFF CLEAR_PIN(LED_ACUM_PORT, LED_ACUM_PIN)
#define LED_ACUM_ON SET_PIN(LED_ACUM_PORT, LED_ACUM_PIN)
#define LED_ACUM_TOGGLE TOGGLE_PIN(LED_ACUM_PORT, LED_ACUM_PIN)

typedef uint32_t timer_t;

typedef enum
{
	EV_SYSTEM_STATE = 0,
	EV_SYSTEM_ERROR_MOTOR,
	EV_SYSTEM_OVF_MOTOR_CNT,
	EV_SYSTEM_ERROR_SERVO,
	EV_SYSTEM_SERVO_TRY,
	EV_SERVO_DISP_FAST_ADD,
	EV_SERVO_DISP_VERY_FAST_ADD,
	EV_MOTOR_DISP_FAST_ADD,
	EV_MOTOR_DISP_VERY_FAST_ADD,
	EV_SERVO_DISP_FAST_SUB,
	EV_SERVO_DISP_VERY_FAST_SUB,
	EV_MOTOR_DISP_FAST_SUB,
	EV_MOTOR_DISP_VERY_FAST_SUB,
	EV_ON_BUZZER,
	EV_ERR_SERVO_HIGH_CURRENT,
	EV_ERR_MOTOR_HIGH_CURRENT = 15
	
}   event_t;

typedef enum
{
	EV_ERR_HIGH_VOLTAGE = 0,
	EV_ERR_LOW_VOLTAGE,
	EV_ERR_VERY_LOW_VOLTAGE,
}event2_t;

#define ERROR_MOTOR_HIGH_CURRENT 1
#define ERROR_SERVO_HIGH_CURRENT 2

#define ANIM_COUNTER_OVF 10
#define VERY_FAST_COUNTER_OVF 15

///////////////////////////////////////////
///UART
#define UART_RX0_BUFFER_SIZE 128
#define UART_TX0_BUFFER_SIZE 128
#define USART0_ENABLED
#define USE_USART 1

///////////////////////////////////////////
///ERROR
#define CONFIG_USE_ERROR_MOTOR 1
#define CONFIG_USE_ERROR_SERVO 1
#define CONFIG_USE_ERROR_VOLTAGE 1
#if CONFIG_DEVICE_SIEWNIK
#define CONFIG_USE_AXELERATE_MOTOR 1
#endif


#define TEST_APP 0
#define L_DEBUG 1
#define SERIAL_PLOT 0
#define DARK_MENU 1
////////////////// ZMNIEJSZANIE JASNOSCI////////////////////
#define DISP_LIGHT 1 // im wieksza wartosz, tym mniejsza jasnosc

#if SERIAL_PLOT && L_DEBUG
#error "Wlacz tylko SERIAL_PLOT lub L_DEBUG";
#endif

#if L_DEBUG
extern void debug_msg( const char* format, ... );
#define LOGI(string) uart_puts(string)
#define LOGI_F(f_, ...) debug_msg((f_), __VA_ARGS__ )
#else
#define LOGI_F(f_, ...)
#define LOGI(string)
#define debug_msg(f_, ...)
#endif


extern uint16_t system_events;
extern uint16_t system_events2;



#endif /* INCFILE1_H_ */