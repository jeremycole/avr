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
#include <rtc.h>
#include <rtc_ds1307.h>

int main(void)
{
  uart_t *u0;
  uint8_t rc;
  rtc_device_t *rtc = &rtc_ds1307;
  rtc_datetime_24h_t current_dt, offset_dt;

  _delay_ms(1000);

  u0 = uart_init("0", UART_BAUD_SELECT(38400, F_CPU));
  uart_init_stdout(u0);

  i2c_init();

  DDRC = _BV(PC0) | _BV(PC1);
  PORTC = _BV(PC0) | _BV(PC1);

  sei();

  printf("\n\nBooted up!\n");

  rc = rtc_init(rtc);
  printf("Inited RTC, rc=%i\n", rc);

  rc = rtc_sqw_rate(rtc, 1);
  printf("Set sqw rate, rc=%i\n", rc);

  rc = rtc_sqw_enable(rtc);
  printf("Set sqw enable, rc=%i\n", rc);

  rc = rtc_clock_start(rtc);
  printf("Started clock, rc=%i\n", rc);

  _delay_ms(1000);

  while(1)
  {
    rc = rtc_read(rtc, &current_dt);

    rtc_offset_time(&current_dt, &offset_dt, -7);

    printf("rc = %i, %04i-%02i-%02i %02i:%02i:%02i %i offset: %04i-%02i-%02i %02i:%02i:%02i %i\n",
        rc,
        current_dt.year, current_dt.month, current_dt.date,
        current_dt.hour, current_dt.minute, current_dt.second,
        current_dt.day_of_week,
        offset_dt.year, offset_dt.month, offset_dt.date,
        offset_dt.hour, offset_dt.minute, offset_dt.second,
        offset_dt.day_of_week
    );

    _delay_ms(1000);
  }

  return(0);
}
