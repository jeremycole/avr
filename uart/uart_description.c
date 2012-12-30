/*
    Copyright (c) 2010, Jeremy Cole <jeremy@jcole.us>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include "uart_description.h"

#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include "uart_quirks.h"

#define UARTX_DESCRIPTION(name, suffix) \
    { name, \
      { \
          &UCSR##suffix##A, \
          &UCSR##suffix##B, \
          &UCSR##suffix##C, \
          &UDR##suffix, \
          &UBRR##suffix##H, \
          &UBRR##suffix##L \
      }, \
      (_BV(RXCIE##suffix) | _BV(RXEN##suffix)| _BV(TXEN##suffix)), \
      _BV(UDRIE##suffix), \
      _BV(U2X##suffix), \
      0, \
      (_BV(UCSZ##suffix##1) | _BV(UCSZ##suffix##0)), \
      _BV(FE##suffix), \
      _BV(DOR##suffix) \
    }


uart_description_t uart_descriptions[] = {

#if defined(UART0_STATUS)
    { "0",
      {
          &UART0_STATUS,
          &UART0_CONTROL,
          &UART0_FORMAT,
          &UART0_DATA,
          &UART0_UBRRH,
          &UART0_UBRRL
      },
      UART0_ENABLE,
      UART0_UDRIE,
      UART0_U2X,
      0,
      UART0_FORMAT_8N1,
      UART0_ERROR_FE,
      UART0_ERROR_DOR
    },
#endif

#if defined(UDRIE1)
    UARTX_DESCRIPTION("1", 1),
#endif

#if defined(UDRIE2)
    UARTX_DESCRIPTION("2", 2),
#endif

#if defined(UDRIE3)
    UARTX_DESCRIPTION("3", 3),
#endif

    { NULL, {NULL, NULL, NULL, NULL, NULL, NULL}, 0, 0, 0, 0, 0, 0 }
};

