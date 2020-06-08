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

typedef struct
{
	uint8_t y_start;
	uint8_t line_max;
	uint8_t all_line;
	uint8_t actual_line;
}scrollBar_t;

int ssdFigureDrawLoadBar(loadBar_t * figure);
int ssdFigureDrawScrollBar(scrollBar_t * figure);
int ssdFigureFillLine(int y_start, int height);
void drawMotor(uint8_t x, uint8_t y);
void drawServo(uint8_t x, uint8_t y, uint8_t open);

#endif