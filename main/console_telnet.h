#ifndef CONSOLE_TELNET_H_
#define CONSOLE_TELNET_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "config.h"
#include "tokenline.h"

#ifndef CONFIG_USE_CONSOLE_TELNET
#define CONFIG_USE_CONSOLE_TELNET FALSE
#endif

#if CONFIG_USE_CONSOLE_TELNET
#include "libtelnet.h"
#include "telnet.h"

extern console_t con0telnet[3];

void consoleEthInit(void);
void consoleTelentStart(void);
void setStatusEthConsole(uint8_t console, uint8_t status);

#endif

#endif /* CONSOLE_TELNET_H_ */