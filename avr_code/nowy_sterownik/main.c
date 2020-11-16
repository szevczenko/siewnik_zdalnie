/*
 * nowy_sterownik.c
 *
 * Created: 24.09.2019 18:07:05
 * Author : Demetriusz
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "system.h"
#include "config.h"
#include "usart.h"
#include "measure.h"
#include "accumulator.h"
#include "servo.h"
#include "dcmotorpwm.h"

#if L_DEBUG
#include <stdio.h>
char debug_buff[64];
void debug_msg( const char* format, ... )
{
	va_list arglist;
	va_start( arglist, format );
	vsprintf(debug_buff, format, arglist );
	va_end( arglist );
	#if USE_USART
	uart_puts(debug_buff);
	#endif
}
#endif

#if SERIAL_PLOT
void uart_process(void)
{
	static timer_t uart_send_timer;
	static uint8_t *pnt;
	static uint16_t send_value;
	if (uart_send_timer < mktime.ms)
	{
		pnt = &send_value;
		send_value = measure_get_filtered_value(MEAS_MOTOR);
		uart_putc(pnt[0]);
		uart_putc(pnt[1]);
		uart_send_timer += 100;
		
	}
}
#endif



uint16_t system_events;
uint8_t motor_value;
uint8_t servo_vibro_value;

void init_pin(void)
{
	//UART_TX_PIN
	DDRD |= (1<<1);
	//UART_RX_PIN
}

void init_driver(void)
{
	#if USE_USART
	uart_init(UART_BAUD_SELECT(57600, F_CPU));
	#endif
	timer0_init(TIM0_PRESCALER, TIM0_ARR);
	init_system();
	init_measure();
	CLEAR_PIN(SFIOR, PUD);
}

int main(void)
{
	#if CONFIG_DEVICE_SIEWNIK
	servo_init(0);
	#endif
	init_pin();
	sei();
	init_driver();
	debug_msg("/-----------START SYSTEM------------/\n");
	on_system();
    /* Replace with your application code */
    while (1) 
    {
		dcmotor_process(motor_value);
		#if CONFIG_DEVICE_SIEWNIK
		servo_process(servo_vibro_value);
		#endif
		measure_process();
		accumulator_process();
		#if SERIAL_PLOT && USE_USART
		uart_process();
		#endif
    }
}

