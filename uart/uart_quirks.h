/*
    Copyright (c) 2010, Jeremy Cole <jeremy@jcole.us>
    Copyright (c) 2006, Peter Fleury <pfleury@gmx.ch>

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

#ifndef UART_QUIRKS_H
#define UART_QUIRKS_H

#include <avr/io.h>

/*
 * Old AVR AT90 classic or ATmega103 with one UART.
 * - Status/Control Registers: Split USR/UCR
 * - Numeric Suffixes on Regs: No
 * - Numeric Suffixes on ISRs: No
 */
#if defined(__AVR_AT90S2313__) \
  || defined(__AVR_AT90S4414__) \
  || defined(__AVR_AT90S4434__) \
  || defined(__AVR_AT90S8515__) \
  || defined(__AVR_AT90S8535__) \
  || defined(__AVR_ATmega103__)

#define UART0_PRESENT   1
#define UART0_RXC_INT   UART_RX_vect
#define UART0_TXC_INT   UART_TX_vect
#define UART0_DRE_INT   UART_UDRE_vect
#define UART0_STATUS    USR
#define UART0_CONTROL   UCR
#define UART0_DATA      UDR
#define UART0_UBRRL     UBRR
#undef  UART0_UBRRH
#define UART0_UDRIE     _BV(UDRIE)
#undef  UART0_U2X
#undef  UART0_ENABLE
#define UART0_FORMAT_8N1 (_BV(UCSZ01) | _BV(UCSZ00))
#define UART0_ERROR_FE  _BV(FE)
#define UART0_ERROR_DOR _BV(DOR)

/*
 * Old AVR AT90 classic with one UART.
 * - Status/Control Registers: Combined A/B
 * - Numeric Suffixes on Regs: No
 * - Numeric Suffixes on ISRs: No
 */
#elif defined(__AVR_AT90S2333__) \
  || defined(__AVR_AT90S4433__)

#define UART0_RXC_INT   UART_RX_vect
#define UART0_TXC_INT   UART_TX_vect
#define UART0_DRE_INT   UART_UDRE_vect
#define UART0_STATUS    UCSRA
#define UART0_CONTROL   UCSRB
#define UART0_FORMAT    UCSRC
#define UART0_DATA      UDR
#define UART0_UBRRL     UBRR
#undef  UART0_UBRRH
#define UART0_UDRIE     _BV(UDRIE)
#undef  UART0_U2X
#undef  UART0_ENABLE
#define UART0_FORMAT_8N1 (_BV(UCSZ1) | _BV(UCSZ0))
#define UART0_ERROR_FE  _BV(FE)
#define UART0_ERROR_DOR _BV(DOR)

/* ATmega with one UART/USART
 * - Status/Control Registers: Combined A/B
 * - Numeric Suffixes on Regs: No
 * - Numeric Suffixes on ISRs: No
 */
#elif  defined(__AVR_ATmega8__)   \
  || defined(__AVR_ATmega16__)    \
  || defined(__AVR_ATmega32__)    \
  || defined(__AVR_ATmega323__)   \
  || defined(__AVR_ATmega8515__)  \
  || defined(__AVR_ATmega8535__)  \
  || defined(__AVR_ATtiny2313__)  \
  || defined(__AVR_ATmega163__)   \
  || defined(__AVR_ATmega169__)

#undef  UART0_RXC_INT
#undef  UART0_TXC_INT
#undef  UART0_DRE_INT
#define UART0_STATUS    UCSRA
#define UART0_CONTROL   UCSRB
#define UART0_FORMAT    UCSRC
#define UART0_DATA      UDR
#if defined(UBRRHI)
/* This is a UART, not a USART */
#define UART0_UBRRL     UBRR
#define UART0_UBRRH     UBRRHI
#else
#define UART0_UBRRL     UBRRL
#define UART0_UBRRH     UBRRH
#endif
#define UART0_UDRIE     _BV(UDRIE)
#define UART0_U2X       _BV(U2X)
#define UART0_ENABLE    (_BV(RXCIE) | _BV(RXEN)| _BV(TXEN))
#define UART0_FORMAT_8N1 (_BV(UCSZ1) | _BV(UCSZ0))
#define UART0_ERROR_FE  _BV(FE)
#define UART0_ERROR_DOR _BV(DOR)

/* ATmega with one USART
 * - Status/Control Registers: Combined A/B
 * - Numeric Suffixes on Regs: Yes
 * - Numeric Suffixes on ISRs: Yes
 */
#elif defined(__AVR_ATmega48__) \
  || defined(__AVR_ATmega88__) \
  || defined(__AVR_ATmega168__) \
  || defined(__AVR_ATmega329__) \
  || defined(__AVR_ATmega3290__) \
  || defined(__AVR_ATmega649__) \
  || defined(__AVR_ATmega6490__) \
  || defined(__AVR_ATmega325__) \
  || defined(__AVR_ATmega3250__) \
  || defined(__AVR_ATmega645__) \
  || defined(__AVR_ATmega6450__) \
  || defined(__AVR_ATmega644__)

#undef  UART0_RXC_INT
#undef  UART0_TXC_INT
#undef  UART0_DRE_INT
#define UART0_STATUS    UCSR0A
#define UART0_CONTROL   UCSR0B
#define UART0_FORMAT    UCSR0C
#define UART0_DATA      UDR0
#define UART0_UBRRL     UBRR0L
#define UART0_UBRRH     UBRR0H
#define UART0_UDRIE     _BV(UDRIE0)
#define UART0_U2X       _BV(U2X0)
#define UART0_ENABLE    (_BV(RXCIE0) | _BV(RXEN0)| _BV(TXEN0))
#define UART0_FORMAT_8N1 (_BV(UCSZ01) | _BV(UCSZ00))
#define UART0_ERROR_FE  _BV(FE0)
#define UART0_ERROR_DOR _BV(DOR0)

/* ATmega with two USART */
#elif defined(__AVR_ATmega162__) \
  || defined(__AVR_ATmega64__) \
  || defined(__AVR_ATmega128__) \
  || defined(__AVR_ATmega2560__) \
  || defined(__AVR_ATmega1280__) \
  || defined(__AVR_ATmega640__) \
  || defined(__AVR_ATmega164P__) \
  || defined(__AVR_ATmega324P__) \
  || defined(__AVR_ATmega644P__) \
  || defined(__AVR_ATmega1284P__)

#undef  UART0_RXC_INT
#undef  UART0_TXC_INT
#undef  UART0_DRE_INT
#define UART0_STATUS    UCSR0A
#define UART0_CONTROL   UCSR0B
#define UART0_FORMAT    UCSR0C
#define UART0_DATA      UDR0
#define UART0_UBRRL     UBRR0L
#define UART0_UBRRH     UBRR0H
#define UART0_UDRIE     _BV(UDRIE0)
#define UART0_U2X       _BV(U2X0)
#define UART0_ENABLE    (_BV(RXCIE0) | _BV(RXEN0)| _BV(TXEN0))
#define UART0_FORMAT_8N1 (_BV(UCSZ01) | _BV(UCSZ00))
#define UART0_ERROR_FE  _BV(FE0)
#define UART0_ERROR_DOR _BV(DOR0)

#else
#error "No UART definition for MCU available"
#endif

#if !defined(USART0_RXC_INT)
#if defined(USART0_RX_vect)
#define UART0_RXC_INT   USART0_RX_vect
#elif defined(USART_RX_vect)
#define UART0_RXC_INT   USART_RX_vect
#elif defined(UART0_RXC_vect)
#define UART0_RXC_INT   USART0_RXC_vect
#elif defined(UART_RXC_vect)
#define UART0_RXC_INT   USART_RXC_vect
#else
#error "Couldn't find a usable interrupt vector for UART0_RXC_INT!"
#endif
#endif

#if !defined(USART0_TXC_INT)
#if defined(USART0_TX_vect)
#define UART0_TXC_INT   USART0_TX_vect
#elif defined(USART_TX_vect)
#define UART0_TXC_INT   USART_TX_vect
#elif defined(UART0_TXC_vect)
#define UART0_TXC_INT   USART0_TXC_vect
#elif defined(UART_TXC_vect)
#define UART0_TXC_INT   USART_TXC_vect
#else
#error "Couldn't find a usable interrupt vector for UART0_TXC_INT!"
#endif
#endif

#if !defined(USART0_DRE_INT)
#if defined(USART0_UDRE_vect)
#define UART0_DRE_INT   USART0_UDRE_vect
#elif defined(USART_UDRE_vect)
#define UART0_DRE_INT   USART_UDRE_vect
#else
#error "Couldn't find a usable interrupt vector for UART0_DRE_INT!"
#endif
#endif

#endif /* UART_QUIRKS_H */
