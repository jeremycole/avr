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

uint8_t i2c_data[16];
uint8_t data_offset;

uint8_t handle_sr(uint8_t status, i2c_mode_t last_mode, i2c_mode_t current_mode)
{
  if(last_mode != current_mode)
  {
    data_offset=0;
    memset(i2c_data, 0, 16);
  }

  if(status == TW_SR_DATA_ACK || status == TW_SR_DATA_NACK)
    i2c_data[data_offset++] = TWDR;

  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
  return 0;
}

int main(void)
{
  uart_t *u0;

  _delay_ms(1000);

  u0 = uart_init("0", 0);
  uart_init_stdout(u0);

  i2c_init();
  i2c_slave_init(0x70, 0, 0);
  i2c_global.sr_callback = handle_sr;

  sei();

  printf("\n\nBooted up!\n");

  while(1)
  {
    printf("I2C Mode: %i, Data: %i, %i, %i\n", i2c_global.mode,
        i2c_data[0], i2c_data[1], i2c_data[2]);

    _delay_ms(1000);
  }

  return(0);
}
