#ifndef _MEM_H
#define _MEM_H
#include "stdint.h"

typedef struct
{
	uint8_t first_disp_val;
	uint8_t second_disp_val;
}mem_program;

typedef enum
{
    MEM_PROGRAM_1,
    MEM_PROGRAM_2,
    MEM_PROGRAM_3,
	MEM_LAST_DATA,
    MEM_CALIBRATION
}_type_data;

void mem_save_data(_type_data param, void *data);
void mem_read_data(_type_data param, void *data);


#endif