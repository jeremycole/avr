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

#include "uart.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 *  Module global variables
 */

uart_instance_t *uart_instances = NULL;
uart_stdout_t uart_stdout;

#include "uart_quirks.h"

#if defined(UART0_RXC_INT) && defined(UART0_DRE_INT)
ISR(UART0_RXC_INT)    { uart_isr_rxc("0"); }
ISR(UART0_DRE_INT)    { uart_isr_dre("0"); }
#endif

#if defined(USART1_RX_vect)
ISR(USART1_RX_vect)   { uart_isr_rxc("1"); }
ISR(USART1_UDRE_vect) { uart_isr_dre("1"); }
#endif

#if defined(USART2_RX_vect)
ISR(USART2_RX_vect)   { uart_isr_rxc("2"); }
ISR(USART2_UDRE_vect) { uart_isr_dre("2"); }
#endif

#if defined(USART3_RX_vect)
ISR(USART3_RX_vect)   { uart_isr_rxc("3"); }
ISR(USART3_UDRE_vect) { uart_isr_dre("3"); }
#endif


inline uart_instance_t *uart_instance_add(uart_t *uart)
{
  uart_instance_t *instance, *current;

  instance = malloc(sizeof(uart_instance_t));

  if(uart_instances == NULL)
  {
    uart_instances = instance;
  } else {
    for(current = uart_instances; current->next; current = current->next) { };
    current->next = instance;
  }
  instance->next = NULL;
  instance->uart = uart;

  return uart_instances;
}

uart_t *uart_by_name(char *name)
{
  uart_instance_t *instance;
  for(instance=uart_instances; instance; instance = instance->next)
  {
    if(!(strcmp(instance->uart->description->name, name)))
      return instance->uart;
  }
  return NULL;
}

uart_description_t *uart_description_by_name(char *name)
{
  uart_description_t *d;
  for(d=uart_descriptions; d; d++)
  {
    if(!(strcmp(d->name, name)))
      return d;
  }
  return NULL;
}

uart_t *uart_init(char *name, unsigned int baudrate)
{
  uart_t *uart;
  uart_description_t *description;

  description = uart_description_by_name(name);

  uart = malloc(sizeof(uart_t));
  uart->tx.buffer = malloc(UART_TX_BUFFER_SIZE);
  uart->rx.buffer = malloc(UART_RX_BUFFER_SIZE);
  uart->tx.head = 0;
  uart->tx.tail = 0;
  uart->rx.head = 0;
  uart->rx.tail = 0;
  uart->rx_callback = NULL;
  uart->description = description;

  uart_set_baudrate(uart, baudrate);

  /* Set frame format: asynchronous, 8data, no parity, 1stop bit */
  uart_set_frame_format(uart,
      uart->description->format_async
    | uart->description->format_8n1);

  /* Enable UART receiver, transmitter, and RX complete interrupt. */
  if(uart->description->control_enable)
  {
    *uart->description->registers.control |= uart->description->control_enable;
  }

  uart_instance_add(uart);

  return uart;
}

void uart_set_baudrate(uart_t *uart, unsigned int baudrate)
{
  if( uart->description->status_u2x && (baudrate & UART_BAUD_RATE_2X) )
  {
    /* Enable 2x speed */
    *uart->description->registers.status = uart->description->status_u2x;
  }

  baudrate &= ~UART_BAUD_RATE_2X;

  /* Set baud rate */
  if(uart->description->registers.baud_h)
  {
    *uart->description->registers.baud_h = (unsigned char) (baudrate >> 8);
  }

  *uart->description->registers.baud_l = (unsigned char) (baudrate & 0x00ff);
}

void uart_set_frame_format(uart_t *uart, int frame_format)
{
  /* TODO: Make this resettable by not just using |= */
  *uart->description->registers.format |= frame_format;
}

void uart_set_rx_callback(uart_t *uart, void (*rx_callback)(uart_t *))
{
  uart->rx_callback = rx_callback;
}

void uart_isr_rxc(char *name)
{
  uart_t *uart;
  unsigned char tmp_rx_head;
  unsigned char data;
  unsigned char usr;
  unsigned char rx_error;

  uart = uart_by_name(name);

  /* Read UART status register and UART data register */
  usr  = *uart->description->registers.status;
  data = *uart->description->registers.data;

  rx_error = (usr & (uart->description->error_fe | uart->description->error_dor) );

  /* calculate buffer index */
  tmp_rx_head = ( uart->rx.head + 1) & UART_RX_BUFFER_MASK;

  if ( tmp_rx_head == uart->rx.tail )
  {
    /* error: receive buffer overflow */
    rx_error = UART_BUFFER_OVERFLOW >> 8;
  } else {
    /* store new index */
    uart->rx.head = tmp_rx_head;
    /* store received data in buffer */
    uart->rx.buffer[tmp_rx_head] = data;
  }
  uart->rx.error = rx_error;

  if(uart->rx_callback)
    uart->rx_callback(uart);
}

void uart_isr_dre(char *name)
{
  uart_t *uart;
  unsigned char tmp_tx_tail;

  uart = uart_by_name(name);

  if ( uart->tx.head != uart->tx.tail)
  {
    /* Calculate and store new buffer index */
    tmp_tx_tail = (uart->tx.tail + 1) & UART_TX_BUFFER_MASK;
    uart->tx.tail = tmp_tx_tail;
    /* Get one byte from buffer and write it to UART */
    *uart->description->registers.data = uart->tx.buffer[tmp_tx_tail];  /* start transmission */
  } else {
    /* TX buffer empty, disable UDRE interrupt */
    *uart->description->registers.control &= ~uart->description->control_udrie;
  }
}

unsigned char uart_data_ready(uart_t *uart)
{
  if (uart->rx.head == uart->rx.tail)
    return 0;
  return 1;
}

unsigned int uart_getc(uart_t *uart)
{
  unsigned char tmp_rx_tail, data;

  if (uart_data_ready(uart) == 0)
  {
    return UART_NO_DATA;   /* no data available */
  }

  /* Calculate new tail of receive buffer */
  tmp_rx_tail = (uart->rx.tail + 1) & UART_RX_BUFFER_MASK;

  /* Get data from new tail of receive buffer */
  data = uart->rx.buffer[tmp_rx_tail];

  /* Adjust tail pointer, allowing writes into buffer to unblock */
  uart->rx.tail = tmp_rx_tail;

  return (uart->rx.error << 8) + data;
}

void uart_putc(uart_t *uart, unsigned char data)
{
  unsigned char tmp_tx_head;

  tmp_tx_head = (uart->tx.head + 1) & UART_TX_BUFFER_MASK;

  while ( tmp_tx_head == uart->tx.tail ){
    /* Wait for free space in buffer. */
  }

  uart->tx.buffer[tmp_tx_head] = data;
  uart->tx.head = tmp_tx_head;

  /* enable UDRE interrupt */
  *uart->description->registers.control |= uart->description->control_udrie;
}

void uart_puts(uart_t *uart, const char *s)
{
  while (*s)
    uart_putc(uart, *s++);
}

void uart_puts_P(uart_t *uart, const char *progmem_s)
{
  register char c;

  while ( (c = pgm_read_byte(progmem_s++)) )
    uart_putc(uart, c);
}

void uart_init_stdout(uart_t *uart)
{
  uart_stdout.uart = uart;
  fdev_setup_stream(&uart_stdout.file, uart_putchar, NULL, _FDEV_SETUP_WRITE);
  stdout = &uart_stdout.file;
  stderr = &uart_stdout.file;
}

int uart_putchar(char c, FILE *stream)
{
  if(c == '\n')
    uart_putchar('\r', stream);

  uart_putc(uart_stdout.uart, c);

  return 0;
}
