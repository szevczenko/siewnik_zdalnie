#include "stdint.h"
#include "eeprom.h"
#include "mem.h"
#include "config.h"

#ifndef DEBUG_MEM
#define DEBUG_MEM (void)
#endif


void mem_save_data(_type_data param, void *data)
{
    int offset;
    int len;
    uint8_t *data_pnt = (uint8_t*)data;
    switch(param)
    {
        case MEM_PROGRAM_1:
        offset = 0;
        len = sizeof(mem_program);
        break;
        case MEM_PROGRAM_2:
        offset = sizeof(mem_program);
        len = sizeof(mem_program);
        break;
        case MEM_PROGRAM_3:
        offset = 2*sizeof(mem_program);
        len = sizeof(mem_program);
        break;
		case MEM_LAST_DATA:
		offset = 3*sizeof(mem_program);
		len = sizeof(mem_program);
		break;
        case MEM_CALIBRATION:
        offset = 4*sizeof(mem_program);
        break;
        default:
        return;
        break;
    }

	for (uint8_t byte_n=offset; byte_n<len + offset;byte_n++)
	{
		//debug_msg("MEM: Write add: %d, data: %d\n", byte_n, *data_pnt);
		EEPROM_write(byte_n,*data_pnt);
		data_pnt++;
	}
}

void mem_read_data(_type_data param, void *data)
{
    int offset = 0;
    int len = 0;
    uint8_t *data_pnt = (uint8_t*)data;
    switch(param)
    {
        case MEM_PROGRAM_1:
        offset = 0;
        len = sizeof(mem_program);
        break;
        case MEM_PROGRAM_2:
        offset = sizeof(mem_program);
        len = sizeof(mem_program);
        break;
        case MEM_PROGRAM_3:
        offset = 2*sizeof(mem_program);
        len = sizeof(mem_program);
        break;
		case MEM_LAST_DATA:
		offset = 3*sizeof(mem_program);
		len = sizeof(mem_program);
		break;
        case MEM_CALIBRATION:
        offset = 4*sizeof(mem_program);
        len = 0;
        break;
        default:
        return;
        break;
    }

	for (uint8_t byte_n=offset; byte_n<len + offset;byte_n++)
	{
		*data_pnt = EEPROM_read(byte_n);
		//debug_msg("MEM: Read add: %d, data: %d\n", byte_n, *data_pnt);
		data_pnt++;
	}
}
