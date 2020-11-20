#ifndef _TOKEN_H_
#define _TOKEN_H_

#include "console.h"
#include "tokenline.h"

#if CONFIG_USE_CONSOLE_TOKEN

typedef int (*cmdfunc)(console_t *con, t_tokenline_parsed *p);

typedef struct
{
	int token;
	cmdfunc func;
} tokenMap_t;

extern tokenMap_t tokenMap[];
extern t_token tl_tokens[];
extern t_token_dict tl_dict[];

#endif //#if CONFIG_USE_CONSOLE_TOKEN

#endif
