/*
    Copyright (c) 2015, Jeremy Cole <jeremy@jcole.us>

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

#ifndef RTC_DS1307_H
#define RTC_DS1307_H

#include <inttypes.h>
#include <avr/io.h>

#include "rtc_types.h"

#define RTC_DS1307_I2C_ID  0xD0

#define RTC_DS1307_YEAR_EPOCH 50

#define RTC_DS1307_CLOCK_HALT 1
#define RTC_DS1307_CLOCK_RUN  0

#define RTC_DS1307_HOUR_STYLE_24H 0
#define RTC_DS1307_HOUR_STYLE_12H 1

#define RTC_DS1307_SQW_DISABLE 0
#define RTC_DS1307_SQW_ENABLE  1

#define RTC_DS1307_SQW_RS1 1
#define RTC_DS1307_SQW_RS0 0

#define RTC_DS1307_SQW_RATE_1HZ     ( 0 )
#define RTC_DS1307_SQW_RATE_4096HZ  ( _BV(RTC_DS1307_SQW_RS0) )
#define RTC_DS1307_SQW_RATE_8192HZ  ( _BV(RTC_DS1307_SQW_RS1) )
#define RTC_DS1307_SQW_RATE_32768HZ ( _BV(RTC_DS1307_SQW_RS1) | _BV(RTC_DS1307_SQW_RS0) )

#define RTC_DS1307_CONTROL_CLOCK_HALT          7
#define RTC_DS1307_CONTROL_HOUR_STYLE          6
#define RTC_DS1307_CONTROL_SQW_RATE            0
#define RTC_DS1307_CONTROL_SQW_OUTPUT_FIXED    7
#define RTC_DS1307_CONTROL_SQW_ENABLE          4

#define RTC_DS1307_REGISTER_CLOCK_HALT         0x00
#define RTC_DS1307_REGISTER_HOUR_STYLE         0x02
#define RTC_DS1307_REGISTER_SQW_RATE           0x07
#define RTC_DS1307_REGISTER_SQW_OUTPUT_FIXED   0x07
#define RTC_DS1307_REGISTER_SQW_ENABLE         0x07

typedef struct _rtc_ds1307_clock_raw_t
{
  uint8_t second_0:4;
  uint8_t second_1:3;
  uint8_t control_clock_halt:1;
  uint8_t minute_0:4;
  uint8_t minute_1:3;
  uint8_t padding0:1;
  uint8_t hour_0:4;
  uint8_t hour_1:2;
  uint8_t control_mode_24h:1;
  uint8_t padding1:1;
  uint8_t day_of_week:3;
  uint8_t padding2:5;
  uint8_t date_0:4;
  uint8_t date_1:2;
  uint8_t padding3:2;
  uint8_t month_0:4;
  uint8_t month_1:1;
  uint8_t padding4:3;
  uint8_t year_0:4;
  uint8_t year_1:4;
  uint8_t control_rs:2;
  uint8_t padding5:2;
  uint8_t control_sqwe:1;
  uint8_t padding6:2;
  uint8_t control_out:1;
} rtc_ds1307_clock_raw_t;

extern rtc_device_t rtc_ds1307;

#endif /* RTC_DS1307_H */
