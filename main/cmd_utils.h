#ifndef CMD_UTILS_
#define CMD_UTILS_

#include "config.h"
#include "console.h"

#if CONFIG_USE_CONSOLE

enum
{
	DEBUG_TOKENLINE = BIT(0),
};

extern uint32_t debug_flags;

void stream_write(console_t *con, const char *data, const uint32_t size);
void cprint(void *user, const char *str);
void cprintf(console_t *con, const char *fmt, ...);
void cprint_len(void *user, const char *str, uint32_t len);

#if CONFIG_USE_CONSOLE_TOKEN
int cmd_clear(console_t *con, t_tokenline_parsed *p);
int cmd_debug(console_t *con, t_tokenline_parsed *p);
void debug_token_dump(console_t *con, t_tokenline_parsed *p);
int test_daq_cmd(console_t *con, t_tokenline_parsed *p);
#endif //CONFIG_USE_CONSOLE_TOKEN

#endif //#if CONFIG_USE_CONSOLE

#endif
