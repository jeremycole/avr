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

#ifndef RTC_H_
#define RTC_H_

#include <inttypes.h>
#include "rtc_types.h"

extern uint8_t rtc_days_in_month[13][2];
extern char *rtc_month_abbreviations[13];
extern char *rtc_dow_names[8];

extern uint8_t rtc_init(rtc_device_t *rtc);
extern uint8_t rtc_clock_start(rtc_device_t *rtc);
extern uint8_t rtc_clock_stop(rtc_device_t *rtc);
extern uint8_t rtc_sqw_enable(rtc_device_t *rtc);
extern uint8_t rtc_sqw_disable(rtc_device_t *rtc);
extern uint8_t rtc_sqw_rate(rtc_device_t *rtc, uint16_t rate);
extern uint8_t rtc_read(rtc_device_t *rtc, rtc_datetime_24h_t *dt);
extern uint8_t rtc_write(rtc_device_t *rtc, rtc_datetime_24h_t *dt);

extern uint8_t rtc_find_dow(uint16_t y, uint8_t m, uint8_t d);
extern int8_t rtc_find_dst_offset(rtc_datetime_24h_t time, rtc_dst_date_t dst_dates[]);
extern uint8_t rtc_offset_time(rtc_datetime_24h_t *from, rtc_datetime_24h_t *to, uint8_t offset_hours);

#endif /* RTC_H_ */
