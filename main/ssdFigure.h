#ifndef SSD_FIGURE_H
#define SSD_FIGURE_H
#include "config.h"

typedef struct ssdFigure
{
	uint8_t x; 
	uint8_t y;
	uint8_t height;
	uint8_t width;
	uint8_t fill;
}loadBar_t;

int ssdFigureDrawLoadBar(loadBar_t * figure);
void drawMotor(uint8_t x, uint8_t y);

#endif