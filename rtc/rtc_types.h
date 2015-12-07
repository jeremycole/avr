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

#ifndef RTC_TYPES_H_
#define RTC_TYPES_H_

#include <inttypes.h>

typedef struct _rtc_datetime_24h_t
{
  int16_t year;
  int8_t  month;
  int8_t  date;
  int8_t  hour;
  int8_t  minute;
  int8_t  second;
  int16_t millisecond;
  int8_t  day_of_week;
} rtc_datetime_24h_t;

typedef struct _rtc_device_t
{
  uint8_t (*init)(void);
  uint8_t (*clock_start)(void);
  uint8_t (*clock_stop)(void);
  uint8_t (*sqw_enable)(void);
  uint8_t (*sqw_disable)(void);
  uint8_t (*sqw_rate)(uint16_t);
  uint8_t (*read)(rtc_datetime_24h_t *);
  uint8_t (*write)(rtc_datetime_24h_t *);
} rtc_device_t;

typedef struct _rtc_dst_date_t
{
  int16_t year;
  int8_t start_month;
  int8_t start_date;
  int8_t end_month;
  int8_t end_date;
} rtc_dst_date_t;

#endif /* RTC_TYPES_H_ */
