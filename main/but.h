/*
 * but.h
 *
 * Created: 05.02.2019 17:20:49
 *  Author: Demetriusz
 */ 


#ifndef BUT_H_
#define BUT_H_

#define BUTTON_CNT 3 //ilo�� przycisk�w
#define TIMER_CNT_TIMEOUT 50 

void init_buttons(void);

#define BUT1_GPIO 5
#define BUT2_GPIO 4
#define BUT3_GPIO 2
#define BUT4_GPIO 0
#define BUT5_GPIO 2
#define BUT6_GPIO 4
#define BUT7_GPIO 1
#define BUT8_GPIO 4
#define BUT9_GPIO 3
#define BUT10_GPIO 0


typedef struct
{
	uint32_t tim_cnt;
	uint8_t state;
	uint8_t value;
	uint8_t gpio;
	void (*rise_callback)(void *button);
	void (*fall_callback)(void *button);
	void (*timer_callback)(void *button);
}but_t;


typedef enum
{
	BUT_UNLOCK,
	BUT_LOCK_TIMER,
	BUT_LOCK,
}   but_state;

extern but_t button1, button2, button3, button4, button5, button6, button7, button8, button9, button10;


#endif /* BUT_H_ */