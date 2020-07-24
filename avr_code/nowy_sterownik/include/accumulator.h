#ifndef ACCUMULATOR_H
#define ACCUMULATOR_H

typedef enum
{
    ACCUM_NORMAL_VOLTAGE,
	ACCUM_HIGH_VOLTAGE,
	ACCUM_LOW_VOLTAGE,
	ACCUM_VERY_LOW_VOLTAGE,
}state_accumulator;

float accum_get_voltage(void);
void accumulator_process(void);
void accum_led_process(void);


#endif