#include "config.h"
#include "dcmotorpwm.h"
#include "servo.h"
#include "accumulator.h"
#include "measure.h"
#include "system.h"
#include "mem.h"

extern uint8_t servo_vibro_value;

extern uint8_t motor_value;

void init_system(void)
{

}

void on_system(void)
{
	dcmotorpwm_init();	
	init_measure();
	system_events = 0;
	SET_PIN(system_events, EV_SYSTEM_STATE);
}

void off_system(void)
{
	dcmotorpwm_deinit();
	#if CONFIG_DEVICE_SIEWNIK
	#endif /* CONFIG_DEVICE_SIEWNIK */
	system_events = 0;
	CLEAR_PIN(system_events, EV_SYSTEM_STATE);
}

void system_error(void)
{

}
