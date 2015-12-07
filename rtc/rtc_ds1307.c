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

#include <i2c.h>
#include "rtc_ds1307.h"

uint8_t rtc_ds1307_hardware_init(void)
{
  uint8_t rc;

  rc = i2c_start(RTC_DS1307_I2C_ID, I2C_WRITE);
  if(rc) return 1;

  rc = i2c_write(0x00);
  if(rc) return 2;

  i2c_stop();

  rc = i2c_rep_start(RTC_DS1307_I2C_ID, I2C_READ);
  if(rc) return 3;

  i2c_read_nak();
  i2c_stop();

  return 0;
}
uint8_t rtc_ds1307_read_ram(uint8_t address, uint8_t length, unsigned char *data)
{
  uint8_t rc;
  uint8_t pos;

  rc = i2c_start(RTC_DS1307_I2C_ID, I2C_WRITE);
  if(rc) return 1;

  rc = i2c_write(address);
  if(rc) return 2;

  i2c_stop();

  rc = i2c_rep_start(RTC_DS1307_I2C_ID, I2C_READ);
  if(rc) return 3;

  for(pos=0; pos < length; pos++, data++)
  {
    *data = (char)i2c_read(pos != (length-1));
  }
  i2c_stop();

  return 0;
}

uint8_t rtc_ds1307_write_ram(uint8_t address, uint8_t length, unsigned char *data)
{
  uint8_t rc;
  uint8_t pos;

  rc = i2c_start(RTC_DS1307_I2C_ID, I2C_WRITE);
  if(rc) return 1;

  rc = i2c_write(address);
  if(rc) return 2;

  for(pos=0; pos < length; pos++, data++)
  {
    rc = i2c_write((uint8_t)*data);
    if(rc)
    {
      i2c_stop();
      return 3;
    }
  }

  i2c_stop();

  return 0;
}

uint8_t rtc_ds1307_read_register(uint8_t address, uint8_t *value)
{
  return rtc_ds1307_read_ram(address, 1, (uint8_t *)value);
}

uint8_t rtc_ds1307_write_register(uint8_t address, uint8_t *value)
{
  return rtc_ds1307_write_ram(address, 1, (uint8_t *)value);
}

uint8_t rtc_ds1307_read_clock_raw(rtc_ds1307_clock_raw_t *raw)
{
  return rtc_ds1307_read_ram(0x00, 8, (unsigned char *)raw);
}

uint8_t rtc_ds1307_write_clock_raw(rtc_ds1307_clock_raw_t *raw)
{
  return rtc_ds1307_write_ram(0x00, 8, (unsigned char *)raw);
}

uint8_t rtc_ds1307_set_register_bit(uint8_t address, uint8_t bit, uint8_t value)
{
  uint8_t rc;
  uint8_t data;

  rc = rtc_ds1307_read_register(address, &data);
  if(rc) return 1;

  if(value)
  {
    if(data & _BV(bit)) return 0;
    data |= _BV(bit);
  }
  else
  {
    if(!(data & _BV(bit))) return 0;
    data &= ~_BV(bit);
  }

  rc = rtc_ds1307_write_register(address, &data);
  if(rc) return 2;

  return 0;
}

uint8_t rtc_ds1307_hour_style(uint8_t style)
{
  return rtc_ds1307_set_register_bit(RTC_DS1307_REGISTER_HOUR_STYLE,
      RTC_DS1307_CONTROL_HOUR_STYLE, style);
}

uint8_t rtc_ds1307_sqw_output_fixed(uint8_t output)
{
  return rtc_ds1307_set_register_bit(RTC_DS1307_REGISTER_SQW_OUTPUT_FIXED,
      RTC_DS1307_CONTROL_SQW_OUTPUT_FIXED, output);
}

uint8_t rtc_ds1307_init(void)
{
  uint8_t rc;

  rc = rtc_ds1307_hardware_init();
  if(rc) return 1;

  rc = rtc_ds1307_hour_style(RTC_DS1307_HOUR_STYLE_24H);
  if(rc) return 2;

  return 0;
}

uint8_t rtc_ds1307_clock_start(void)
{
  return rtc_ds1307_set_register_bit(RTC_DS1307_REGISTER_CLOCK_HALT,
      RTC_DS1307_CONTROL_CLOCK_HALT, RTC_DS1307_CLOCK_RUN);
}

uint8_t rtc_ds1307_clock_stop(void)
{
  return rtc_ds1307_set_register_bit(RTC_DS1307_REGISTER_CLOCK_HALT,
      RTC_DS1307_CONTROL_CLOCK_HALT, RTC_DS1307_CLOCK_HALT);
}

uint8_t rtc_ds1307_sqw_enable(void)
{
  return rtc_ds1307_set_register_bit(RTC_DS1307_REGISTER_SQW_ENABLE,
      RTC_DS1307_CONTROL_SQW_ENABLE, RTC_DS1307_SQW_ENABLE);
}

uint8_t rtc_ds1307_sqw_disable(void)
{
  return rtc_ds1307_set_register_bit(RTC_DS1307_REGISTER_SQW_ENABLE,
      RTC_DS1307_CONTROL_SQW_ENABLE, RTC_DS1307_SQW_DISABLE);
}

uint8_t rtc_ds1307_sqw_rate(uint16_t rate)
{
  uint8_t rc;
  uint8_t data;
  uint8_t rate_control_code;

  switch(rate)
  {
    case 1:
      rate_control_code = RTC_DS1307_SQW_RATE_1HZ;
      break;
    case 4096:
      rate_control_code = RTC_DS1307_SQW_RATE_4096HZ;
      break;
    case 8192:
      rate_control_code = RTC_DS1307_SQW_RATE_8192HZ;
      break;
    case 32768:
      rate_control_code = RTC_DS1307_SQW_RATE_32768HZ;
      break;
    default:
      return 1;
  }

  rc = rtc_ds1307_read_register(RTC_DS1307_REGISTER_SQW_RATE, &data);
  if(rc) return 1;

  data &= ~(3 << RTC_DS1307_CONTROL_SQW_RATE);
  data |= (rate_control_code & 3) << RTC_DS1307_CONTROL_SQW_RATE;

  rc = rtc_ds1307_write_register(RTC_DS1307_REGISTER_SQW_RATE, &data);
  if(rc) return 2;

  return 0;
}

uint8_t rtc_ds1307_read(rtc_datetime_24h_t *dt)
{
  uint8_t rc;
  rtc_ds1307_clock_raw_t raw;

  rc = rtc_ds1307_read_clock_raw(&raw);
  if(rc) return rc;

  if(raw.control_mode_24h == RTC_DS1307_HOUR_STYLE_12H)
    return 100;

  dt->year   = (raw.year_1   * 10) + raw.year_0;
  dt->month  = (raw.month_1  * 10) + raw.month_0;
  dt->date   = (raw.date_1   * 10) + raw.date_0;
  dt->hour   = (raw.hour_1   * 10) + raw.hour_0;
  dt->minute = (raw.minute_1 * 10) + raw.minute_0;
  dt->second = (raw.second_1 * 10) + raw.second_0;
  dt->millisecond = 0; /* Not supported by this RTC */
  dt->day_of_week = raw.day_of_week;

  dt->year += (dt->year < RTC_DS1307_YEAR_EPOCH)?2000:1900;

  return 0;
}

uint8_t rtc_ds1307_write(rtc_datetime_24h_t *dt)
{
  uint8_t rc;
  rtc_ds1307_clock_raw_t raw;
  uint16_t tmp_year;

  rc = rtc_ds1307_read_clock_raw(&raw);
  if(rc) return rc;

  if(raw.control_mode_24h == RTC_DS1307_HOUR_STYLE_12H)
    return 100;

  tmp_year = dt->year - ((dt->year>=2000)?2000:1900);
  raw.year_0 = tmp_year % 10;
  raw.year_1 = (tmp_year - raw.year_0) / 10;

  raw.month_0 = dt->month % 10;
  raw.month_1 = (dt->month - raw.month_0) / 10;

  raw.date_0 = dt->date % 10;
  raw.date_1 = (dt->date - raw.date_0) / 10;

  raw.hour_0 = dt->hour % 10;
  raw.hour_1 = (dt->hour - raw.hour_0) / 10;

  raw.minute_0 = dt->minute % 10;
  raw.minute_1 = (dt->minute - raw.minute_0) / 10;

  raw.second_0 = dt->second % 10;
  raw.second_1 = (dt->second - raw.second_0) / 10;

  raw.day_of_week = dt->day_of_week;

  rc = rtc_ds1307_write_clock_raw(&raw);
  if(rc) return rc;

  return 0;
}

rtc_device_t rtc_ds1307 = {
  .init         = rtc_ds1307_init,
  .clock_start  = rtc_ds1307_clock_start,
  .clock_stop   = rtc_ds1307_clock_stop,
  .sqw_enable   = rtc_ds1307_sqw_enable,
  .sqw_disable  = rtc_ds1307_sqw_disable,
  .sqw_rate     = rtc_ds1307_sqw_rate,
  .read         = rtc_ds1307_read,
  .write        = rtc_ds1307_write
};
