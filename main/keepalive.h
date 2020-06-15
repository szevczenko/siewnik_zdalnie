#ifndef _KEEP_ALIVE_H
#define _KEEP_ALIVE_H

#define KEEP_ALIVE_TIMEOUT 2000
#define KEEP_ALIVE_TRY 2
#define KEEP_ALIVE_TIME_TO_NEXT 500

void keepAliveInit(int (*send)(uint8_t * data, uint32_t dataLen), void (*errorCb)(void));
void keepAliveAccept(void);
int keepAliveCheckError(void);
void keepAliveStartTask(void);
void keepAliveStart(void);
void keepAliveStop(void);

#endif