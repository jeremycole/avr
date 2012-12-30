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
#include <i2c.h>

int main(void)
{
  uart_t *u0;
  uint8_t x = 0;

  _delay_ms(1000);

  u0 = uart_init("0", 0);
  uart_init_stdout(u0);

  i2c_init();

  DDRC = _BV(PC0) | _BV(PC1);
  PORTC = _BV(PC0) | _BV(PC1);

  sei();

  printf("\n\nBooted up!\n");

  while(1)
  {
    printf("I2C Mode: %i\n", i2c_global.mode);
    if(0 == i2c_start(0x70, I2C_WRITE))
    {
      i2c_write(x++);
      i2c_write(x++);
      i2c_write(x++);
    }
    i2c_stop();

    _delay_ms(1000);
  }

  return(0);
}
