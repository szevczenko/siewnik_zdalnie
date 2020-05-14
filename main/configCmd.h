#ifndef CONFIG_CMD_H_
#define CONFIG_CMD_H_

#include "console.h"

void configRebootToBlt(void);
#if CONFIG_USE_CONSOLE
int configCmd(console_t *con, t_tokenline_parsed *p);
int configEcho(console_t *con, t_tokenline_parsed *p);
int configReset(console_t *con, t_tokenline_parsed *p);
int configMQTT(console_t *con, t_tokenline_parsed *p);
#endif

#endif //CONFIG_CMD_H_