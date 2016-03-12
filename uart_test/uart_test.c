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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <uart.h>

void uart_dump()
{
  uart_description_t *d;
  for(d=uart_descriptions; d->name; d++)
  {
    printf("UART: %s\n", d->name);
    printf("  status:  %02x    u2x:    %02x\n",
        (unsigned int)d->registers.status,
        d->status_u2x);
    printf("  control: %02x    enable: %02x    udrie: %02x\n",
        (unsigned int)d->registers.control,
        d->control_enable,
        d->control_udrie);
    printf("  format:  %02x    8n1:    %02x\n",
        (unsigned int)d->registers.format,
        d->format_8n1);
    printf("  data:    %02x\n", (unsigned int)d->registers.data);
    printf("  baud_h:  %02x\n", (unsigned int)d->registers.baud_h);
    printf("  baud_l:  %02x\n", (unsigned int)d->registers.baud_l);
    printf("\n");
  }
}

int main(void)
{
  uart_t *u0;
  //uart_t *u1;
  _delay_ms(1000);

  u0 = uart_init("0", UART_BAUD_SELECT(9600, F_CPU));
  //u1 = uart_init("1", 0);
  uart_init_stdout(u0);

  sei();

  uart_puts(u0, "\r\n\r\nBooted up!\r\n");

  uart_dump();

  while(1)
  {
    uart_puts(u0, "Test on UART 0!\r\n");
    //uart_puts(u1, "Test on UART 1!\r\n");
    _delay_ms(100);
  }

  return(0);
}
