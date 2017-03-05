#ifndef __MY_UART_H__
#define __MY_UART_H__

#include "defines.h"

void uart_init(void);
void uart_debug(const char *fmt, ...);

#endif
