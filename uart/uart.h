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

/** 
 *  @defgroup pfleury_uart UART Library
 *  @code #include <uart.h> @endcode
 * 
 *  @brief Interrupt UART library using the built-in UART with transmit and receive circular buffers. 
 *
 *  This library can be used to transmit and receive data through the built in UART. 
 *
 *  An interrupt is generated when the UART has finished transmitting or
 *  receiving a byte. The interrupt handling routines use circular buffers
 *  for buffering received and transmitted data.
 *
 *  @note Based on Atmel Application Note AVR306
 *  @author Peter Fleury pfleury@gmx.ch  http://jump.to/fleury
 */
 
/**@{*/

#ifndef UART_H
#define UART_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <avr/io.h>

#include "uart_description.h"

#if (__GNUC__ * 100 + __GNUC_MINOR__) < 304
#error "This library requires AVR-GCC 3.4 or later, update to newer AVR-GCC compiler !"
#endif

typedef struct _uart_buffer_t
{
  volatile unsigned char *buffer;
  volatile unsigned char head;
  volatile unsigned char tail;
  volatile unsigned char error;
} uart_buffer_t;


typedef struct _uart_t
{
  uart_description_t *description;
  uart_buffer_t tx;
  uart_buffer_t rx;
  void (*rx_callback)(struct _uart_t *);
} uart_t;

typedef struct _uart_stdout_t
{
  uart_t *uart;
  FILE file;
} uart_stdout_t;

typedef struct _uart_instance_t
{
  uart_t *uart;
  struct _uart_instance_t *next;
} uart_instance_t;

extern uart_instance_t *uart_instances;
extern uart_stdout_t uart_stdout;

extern uart_t *uart_by_name(char *name);
extern uart_description_t *uart_description_by_name(char *name);

extern void uart_init_stdout(uart_t *uart);
extern int uart_putchar(char c, FILE *stream);

extern uart_t *uart_init(char *name, unsigned int baudrate);
extern void uart_set_baudrate(uart_t *uart, unsigned int baudrate);
extern void uart_set_frame_format(uart_t *uart, int frame_format);
extern void uart_set_rx_callback(uart_t *uart, void (*rx_callback)(uart_t *));
extern void uart_isr_rxc(char *name);
extern void uart_isr_dre(char *name);
extern unsigned char uart_data_ready(uart_t *uart);
extern unsigned int uart_getc(uart_t *uart);
extern void uart_putc(uart_t *uart, unsigned char data);
extern void uart_puts(uart_t *uart, const char *s);
extern void uart_puts_P(uart_t *uart, const char *progmem_s);

/*
** constants and macros
*/

/** The high bit in the baud rate is used to indicate that double speed
 *  should be used when initializing the UART.
 */
#define UART_BAUD_RATE_2X     0x8000

/** @brief  UART Baudrate Expression
 *  @param  xtalcpu  system clock in Mhz, e.g. 4000000L for 4Mhz          
 *  @param  baudrate baudrate in bps, e.g. 1200, 2400, 9600     
 */
#define UART_BAUD_SELECT(baud_rate, xtal_cpu) \
  ((xtal_cpu) / ((baud_rate) * 16l) - 1)

/** @brief  UART Baudrate Expression for ATmega double speed mode
 *  @param  xtalcpu  system clock in Mhz, e.g. 4000000L for 4Mhz           
 *  @param  baudrate baudrate in bps, e.g. 1200, 2400, 9600     
 */
#define UART_BAUD_SELECT_DOUBLE_SPEED(baud_rate, xtal_cpu) \
  (((xtal_cpu) / ((baud_rate) * 8l) - 1) | UART_BAUD_RATE_2X)


/** Default size of the circular receive buffers, must be power of 2 */
#define UART_RX_BUFFER_SIZE 32

/** Default size of the circular transmit buffers, must be power of 2 */
#define UART_TX_BUFFER_SIZE 32

/* size of RX/TX buffers */
#define UART_RX_BUFFER_MASK  ( UART_RX_BUFFER_SIZE - 1)
#define UART_TX_BUFFER_MASK  ( UART_TX_BUFFER_SIZE - 1)

#if ( UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK )
#error "UART_RX_BUFFER_SIZE must be a power of two!"
#endif
#if ( UART_TX_BUFFER_SIZE & UART_TX_BUFFER_MASK )
#error "UART_TX_BUFFER_SIZE must be a power of two!"
#endif

/* Test if the size of the circular buffers fits into SRAM */
#if ( (UART_RX_BUFFER_SIZE+UART_TX_BUFFER_SIZE) >= (RAMEND-0x60 ) )
#error "Size of UART_RX_BUFFER_SIZE + UART_TX_BUFFER_SIZE larger than size of SRAM"
#endif

/* 
** High byte error return code of uart_getc()
*/
#define UART_FRAME_ERROR      0x0800              /* Framing Error by UART       */
#define UART_OVERRUN_ERROR    0x0400              /* Overrun condition by UART   */
#define UART_BUFFER_OVERFLOW  0x0200              /* Receive ringbuffer overflow */
#define UART_NO_DATA          0x0100              /* No receive data available   */

/**@}*/

#endif // UART_H 

