#include <stdint.h>
#include <stdio.h>
#include "accumulator.h"
#include "measure.h"
#include "config.h"

#define ACCUMULATOR_SIZE_TAB 20
#define ACCUMULATOR_ADC_CH 0 //0.028 [V/adc]
#define ACCUMULATOR_HIGH_VOLTAGE 600
#define ACCUMULATOR_LOW_VOLTAGE 395
#define ACCUMULATOR_VERY_LOW_VOLTAGE 350

static float voltage = 0;

float accum_get_voltage(void)
{
	#if CONFIG_DEVICE_SOLARKA
    voltage = measure_get_filtered_value(MEAS_ACCUM)*5*5.7/1024 + 0.7;
	#else
	voltage = measure_get_filtered_value(MEAS_ACCUM)*5*5.7/1024;
	#endif
    return voltage;
}

static uint16_t filtered_accum_adc_val;

void accumulator_process(void)
{
	
	static timer_t accumulator_timer;
	
	if(accumulator_timer < mktime.ms)
	{
		filtered_accum_adc_val = measure_get_filtered_value(MEAS_ACCUM);
		accumulator_timer = mktime.ms + 100;
	}
}
