#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "cmd_utils.h"
#include "token.h"
#include "driver/uart.h"


#if CONFIG_USE_CONSOLE

uint32_t debug_flags = 0;


extern t_token_dict tl_dict[];

//static UBaseType_t uxSavedInterruptStatus;

void stream_write(console_t *con, const char *data, const uint32_t size)
{

	if (!data)
		return;

	if (!size)
		return;
		//Wysyła na wszystkie konsole
	if (con == NULL)
	{
		#if CONFIG_USE_CONSOLE_USB
		#endif //CONFIG_USE_CONSOLE_USB

		#if CONFIG_USE_CONSOLE_SERIAL
		uart_write_bytes(con0serial.sd, data, size);
		#endif

		#if CONFIG_USE_CONSOLE_TELNET
		telnetSendToAll(data, size);
		#endif

		return;
	}
	//Sprawdzenie czy bufor USB nie jest pełny, żeby nie alokować dodatkowej pamięcie i nie wywołać "stack overflow"
	
	switch (con->console_type)
	{
#if CONFIG_USE_CONSOLE_USB
#endif //CONFIG_USE_CONSOLE_USB
#if CONFIG_USE_CONSOLE_SERIAL
	case CON_TYPE_SERIAL:
		uart_write_bytes(con->sd, data, size);
		break;
#endif
#if CONFIG_USE_CONSOLE_CAN
	case CON_TYPE_CAN:
		canConsoleSendData((char *)data, size);
		break;
#endif

#if CONFIG_USE_CONSOLE_TELNET
	case CON_TYPE_TELNET:
		if (*con->telnet != NULL) telnet_send(*con->telnet, data, size);
	break;
	#endif

	}
	
}

void cprint(void *user, const char *str)
{
	console_t *con;
	int len;

	if (!str)
		return;

	con = user;
	len = strlen(str);
	stream_write(con, str, len);
}

void cprint_len(void *user, const char *str, uint32_t len)
{
	console_t *con;

	if (!str)
		return;

	con = user;
	stream_write(con, str, len);
}

void cprintf(console_t *con, const char *fmt, ...)
{
	va_list va_args;
	int real_size;
#define CPRINTF_BUFF_SIZE (511)
	char cprintf_buff[CPRINTF_BUFF_SIZE + 1];

	va_start(va_args, fmt);
	real_size = vsnprintf(cprintf_buff, CPRINTF_BUFF_SIZE, fmt, va_args);
	va_end(va_args);

	stream_write(con, cprintf_buff, real_size);
}

int cmd_clear(console_t *con, t_tokenline_parsed *p)
{
	(void)p;

	cprint(con, "\033[2J"); // ESC seq to clear entire screen
	cprint(con, "\033[H");  // ESC seq to move cursor at left-top corner

	return TRUE;
}

#if CONFIG_XCP_USE_USER_DAQ && CONFIG_USE_XCP
extern void test_daq(void);
#endif
int test_daq_cmd(console_t *con, t_tokenline_parsed *p)
{
	(void)con;
	(void)p;
	#if CONFIG_XCP_TEST_USER_DAQ
	test_daq();
	#else
	cprint(con, "CONFIG_XCP_TEST_USER_DAQ is not active");
	#endif
	return TRUE;
}

#if CONFIG_USE_CONSOLE_TOKEN_DEBUG

void debug_token_dump(console_t *con, t_tokenline_parsed *p)
{
	float arg_float;
	uint32_t arg_uint;
	int i;

	for (i = 0; p->tokens[i]; i++)
	{
		cprintf(con, "%d: ", i);
		switch (p->tokens[i])
		{
		case T_ARG_UINT:
			memcpy(&arg_uint, p->buf + p->tokens[++i], sizeof(uint32_t));
			cprintf(con, "T_ARG_UINT\r\n%d: uint32_t %u\r\n", i, arg_uint);
			break;
		case T_ARG_FLOAT:
			memcpy(&arg_float, p->buf + p->tokens[++i], sizeof(float));
			cprintf(con, "T_ARG_FLOAT\r\n%d: float %f\r\n", i, arg_float);
			break;
		case T_ARG_STRING:
			i++;
			cprintf(con, "T_ARG_STRING\r\n%d: string '%s'\r\n", i, p->buf + p->tokens[i]);
			break;
		case T_ARG_TOKEN_SUFFIX_INT:
			memcpy(&arg_uint, p->buf + p->tokens[++i], sizeof(uint32_t));
			cprintf(con, "T_ARG_TOKEN_SUFFIX_INT\r\n%d: token-suffixed uint32_t %u\r\n", i, arg_uint);
			break;
		default:
			cprintf(con, "token %d (%s)\r\n", p->tokens[i],
					tl_dict[p->tokens[i]].tokenstr);
		}
	}
}

int cmd_debug(console_t *con, t_tokenline_parsed *p)
{
	uint32_t tmp_debug;
	int action, t;

	tmp_debug = 0;
	action = 0;
	for (t = 0; p->tokens[t]; t++)
	{
		switch (p->tokens[t])
		{
		case T_TOKENLINE:
			tmp_debug |= DEBUG_TOKENLINE;
			break;
		case T_ON:
		case T_OFF:
			action = p->tokens[t];
			break;
		}
	}
	if (tmp_debug && !action)
	{
		cprintf(con, "Please specify either 'on' or 'off'.\r\n");
		return FALSE;
	}
	if (action == T_ON)
		debug_flags |= tmp_debug;
	else
		debug_flags &= ~tmp_debug;

	return TRUE;
}
#else
void debug_token_dump(console_t *con, t_tokenline_parsed *p)
{
	(void)con;
	(void)p;
}

#endif // CONFIG_USE_CONSOLE_TOKEN_DEBUG

#endif