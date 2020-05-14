/*
 * but.c
 *
 * Created: 05.02.2019 17:20:37
 *  Author: Demetriusz
 */ 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "config.h"
#include "stdint.h"
#include "but.h"


extern uint8_t test_number;

but_t button1, button2, button3, button4, button5, button6, button7, button8, button9, button10;
but_t *but_tab[BUTTON_CNT] = {&button1, &button2, &button3};//, &button4, &button5, &button6, &button7, &button8, &button9, &button10};
extern time_t mktime;

uint8_t read_button(but_t *but)
{
	return gpio_get_level(but->gpio);
}

extern uint8_t test_button;
void test_fnc(void * pv)
{
}

void init_but_struct(void)
{
	button1.state = 0;
	button1.value = 1;
	button1.fall_callback = 0;
	button1.rise_callback = 0;
	button1.timer_callback = 0;//test_fnc;
	button1.gpio = BUT1_GPIO;
	
	button2.state = 0;
	button2.value = 1;
	button2.fall_callback = 0;
	button2.rise_callback = 0;
	button2.timer_callback = 0;
	button2.gpio = BUT2_GPIO;
	
	button3.state = 0;
	button3.value = 1;
	button3.fall_callback = 0;
	button3.rise_callback = 0;
	button3.timer_callback = 0;
	button3.gpio = BUT3_GPIO;
	
	button4.state = 0;
	button4.value = 1;
	button4.fall_callback = 0;
	button4.rise_callback = 0;
	button4.timer_callback = 0;
	button4.gpio = BUT4_GPIO;
	
	button5.state = 0;
	button5.value = 1;
	button5.fall_callback = 0;
	button5.rise_callback = 0;
	button5.timer_callback = 0;
	button5.gpio = BUT5_GPIO;
	
	button6.state = 0;
	button6.value = 1;
	button6.fall_callback = 0;
	button6.rise_callback = 0;
	button6.timer_callback = 0;
	button6.gpio = BUT6_GPIO;
	
	button7.state = 0;
	button7.value = 1;
	button7.fall_callback = 0;
	button7.rise_callback = 0;
	button7.timer_callback = 0;
	button7.gpio = BUT7_GPIO;
	
	button8.state = 0;
	button8.value = 1;
	button8.fall_callback = 0;
	button8.rise_callback = 0;
	button8.timer_callback = 0;
	button8.gpio = BUT8_GPIO;
	
	button9.state = 0;
	button9.value = 1;
	button9.fall_callback = 0;
	button9.rise_callback = 0;
	button9.timer_callback = 0;
	button9.gpio = BUT9_GPIO;
	
	button10.state = 0;
	button10.value = 1;
	button10.fall_callback = 0;
	button10.rise_callback = 0;
	button10.timer_callback = 0;
	button10.gpio = BUT10_GPIO;
}

static void process_button(void * arg)
{
	uint8_t red_val = 0;
	while(1)
	{
		//process
		for (uint8_t i=0; i<BUTTON_CNT; i++)
		{
			red_val = read_button(but_tab[i]);
			if(red_val != but_tab[i]->value)
			{
				but_tab[i]->value = red_val;
				if (red_val == 1 && but_tab[i]->rise_callback != 0)
				but_tab[i]->rise_callback(but_tab[i]);
				else if(red_val == 0 && but_tab[i]->fall_callback != 0)
				but_tab[i]->fall_callback(but_tab[i]);
			}
			//timer
			if (red_val == 0)
			{
				but_tab[i]->tim_cnt++;
				if (but_tab[i]->tim_cnt >=TIMER_CNT_TIMEOUT && but_tab[i]->state != 1)
				{
					if (but_tab[i]->timer_callback != 0)
					but_tab[i]->timer_callback(&button1);
					but_tab[i]->tim_cnt = 0;
					but_tab[i]->state = 1;
				}
			}
			else
			{
				but_tab[i]->tim_cnt = 0;
				but_tab[i]->state = 0;
			}
		} // end for
		vTaskDelay(20 / portTICK_RATE_MS);
	}// end while
}

static void set_bit_mask(uint32_t *mask)
{
	for(uint8_t i = 0; i < BUTTON_CNT; i++)
	{
		*mask |= (1ULL<<but_tab[i]->gpio);
	}
}

void init_buttons(void)
{
	gpio_config_t io_conf;
	init_but_struct();
	//interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
	set_bit_mask(&io_conf.pin_bit_mask);
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
	xTaskCreate(process_button, "gpio_task", 2048, NULL, 10, NULL);
}

/*
		red_val = read_button(&button1);
		if(red_val != button1.value) 
		{
			button1.value = red_val;
			if (red_val == 1 && button1.rise_callback != 0)
				button1.rise_callback(&button1);
			else if(red_val == 0 && button1.fall_callback != 0)
				button1.fall_callback(&button1);
		}
		//timer
		if (red_val == 0)
		{
			button1.tim_cnt++;
			if (button1.tim_cnt >=50 && button1.state != 1)
			{
				if (button1.timer_callback != 0)
					button1.timer_callback(&button1);
				button1.tim_cnt = 0;
				button1.state = 1;
			}
		}
		else 
		{
			button1.tim_cnt = 0;
			button1.state = 0;
		}
		*/