#include "myuart.h"

#include <util/setbaud.h>

void uart_init(void) {
#ifdef UART_DEBUG_ON
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
#if USE_2X
    UCSR0A |= _BV(U2X0);
#else
    UCSR0A &= ~(_BV(U2X0));
#endif
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8-bit data */
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);   /* Enable RX and TX */
#endif
}

void uart_debug(const char *fmt, ...)
{
#ifdef UART_DEBUG_ON
    static char msg[128];
    va_list ap;
    va_start(ap, fmt);
    int i=0, n=vsprintf(msg, fmt, ap);
    va_end(ap);
    while(i != n) {
        loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
        UDR0 = msg[i++];
    }
#endif
}
