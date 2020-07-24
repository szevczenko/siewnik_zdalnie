#ifndef _SYSTEM_H_
#define _SYSTEM_H_

typedef enum
{
    ERR_REASON_NO,
    ERR_REASON_MOTOR,
    ERR_REASON_SERVO,
	ERR_REASON_TEMPERATURE
}error_reason_;

void on_system(void);
void init_system(void);
void disp_fast_process(void);
void disp_very_fast_process(void);

void system_error(void);

#endif