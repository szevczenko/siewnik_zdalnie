#include <stdint.h>
#include "measure.h"
#include "config.h"
#include "adc.h"

//600
#define SERVO_CALIBRATION_VALUE 600 

////////////////////////////////////////////////
/// ACCUM
static uint8_t iteration_adc_accum_table = 0;
static uint16_t accum_adc;
static uint16_t filtered_accum_adc_val;
static uint16_t accumulator_tab[ACCUMULATOR_SIZE_TAB];

////////////////////////////////////////////////
/// MOTOR
static uint8_t iteration_adc_motor_table = 0;
static uint16_t motor_filter_value;
static uint16_t motor_f_table[FILTER_TABLE_SIZE];
static uint16_t motor_adc;

////////////////////////////////////////////////
/// SERVO OR TEMPERATURE !!! s_o_t - servo or temperature
static uint16_t s_o_t_filter_value;
static uint16_t s_o_t_f_table[FILTER_TABLE_S_SIZE];
static uint8_t s_o_t_iteration_adc_table = 0;
static uint16_t s_o_t_adc;


static uint16_t filtered_value(uint16_t *tab, uint8_t size)
{
	uint16_t ret_val = *tab;
	for (uint8_t i = 1; i<size; i++)
	{
		ret_val = (ret_val + tab[i])/2;
	}
	return ret_val;
}

void init_measure(void)
{
	for(uint8_t i = 0; i<ACCUMULATOR_SIZE_TAB; i++)
	{
		accumulator_tab[i] = ACCUMULATOR_LOW_VOLTAGE + (ACCUMULATOR_HIGH_VOLTAGE - ACCUMULATOR_LOW_VOLTAGE)/2;
	}
    for(uint8_t i = 0; i<FILTER_TABLE_SIZE; i++)
	{
		motor_f_table[i] = 0;
	}
    for(uint8_t i = 0; i<FILTER_TABLE_S_SIZE; i++)
	{
		s_o_t_f_table[i] = 0;
	}
}
static timer_t measure_timer;
static uint32_t debug_msg_counter;
void measure_process(void)
{
	if (measure_timer < mktime.ms)
	{
		measure_timer = mktime.ms + 20;
		accum_adc = adc_read(ACCUMULATOR_ADC_CH); 
		#if CONFIG_DEVICE_SOLARKA
		#endif
		#if CONFIG_DEVICE_SIEWNIK
		accum_adc += motor_filter_value*0.27; //motor_filter_value*0.0075*1025/5/5.7
		#endif
		accumulator_tab[iteration_adc_accum_table] = accum_adc;
		
		iteration_adc_accum_table++;
		motor_adc = adc_read(MOTOR_ADC_CH);
		if (motor_adc > 31) motor_adc = motor_adc - 31;
		else motor_adc = 0;
		motor_f_table[iteration_adc_motor_table] = motor_adc;
		///////////////////////////////////////////////////////////
		////////// TODO isset_timer
		s_o_t_adc = adc_read(SERVO_ADC_CH);

		s_o_t_f_table[s_o_t_iteration_adc_table] = s_o_t_adc;
		iteration_adc_motor_table++;
		s_o_t_iteration_adc_table++;
		filtered_accum_adc_val = filtered_value(accumulator_tab, ACCUMULATOR_SIZE_TAB);
		motor_filter_value = filtered_value(motor_f_table, FILTER_TABLE_SIZE);
		s_o_t_filter_value = filtered_value(s_o_t_f_table, FILTER_TABLE_S_SIZE);
		debug_msg_counter++;
		
		if (iteration_adc_accum_table == ACCUMULATOR_SIZE_TAB) iteration_adc_accum_table = 0;
		if (s_o_t_iteration_adc_table == FILTER_TABLE_S_SIZE) s_o_t_iteration_adc_table = 0;
		if (iteration_adc_motor_table == FILTER_TABLE_SIZE) iteration_adc_motor_table = 0;
	}
}

uint16_t measure_get_filtered_value(_type_measure type)
{
    switch(type)
    {
        case MEAS_ACCUM:
        return filtered_accum_adc_val;
        break;

        case MEAS_MOTOR:
        return motor_filter_value;
        break;

        case MEAS_SERVO:
		case MEAS_TEMPERATURE:
        return s_o_t_filter_value;
        break;
    }
	return 0;
}

uint16_t measure_get_value(_type_measure type)
{
    switch(type)
    {
        case MEAS_ACCUM:
        return accum_adc;
        break;

        case MEAS_MOTOR:
        return motor_adc;
        break;

        case MEAS_SERVO:
		case MEAS_TEMPERATURE:
        return s_o_t_adc;
        break;
    }
	return 0;
}

float measure_get_current(_type_measure type, float resistor)
{
	uint32_t adc;
	switch(type)
	{
		case MEAS_ACCUM:
		adc = filtered_accum_adc_val;
		break;

		case MEAS_MOTOR:
		adc = motor_filter_value;
		break;

		case MEAS_SERVO:
		case MEAS_TEMPERATURE:
		adc = s_o_t_filter_value;
		break;
		
		default:
		adc = 0;
		break;
	}
	float volt = (float) adc / (float) ADC_REFRES * 5.0 /* Volt */;
	return volt / resistor;
}