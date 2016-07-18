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

/*

LCD I2C Digital Clock
=====================

An LCD digital clock controlled by I2C, using LCD Backpack, initially
designed to be connected to LED Analog Clock.  This code expects an
rtc_datetime_24h_t to be transmitted after an I2C START.  For example,
from the I2C master-side, the following code will pass a (locally
created) rtc_datetime_24h_t structure to the LCD I2C Digital Clock for
display:

  void write_remote_lcd(rtc_datetime_24h_t *dt)
  {
    if(0 == i2c_start(0x70, I2C_WRITE))
    {
      i2c_write_array((uint8_t *)dt, 8);
    }
    i2c_stop();
  }

In the future, a more robust protocol supporting many more features could
be implemented by collecting a byte "register" before accepting more data.

*/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define I2C_SCL_CLOCK 400000L
#include <i2c.h>
#include <uart.h>
#include <lcd.h>
#include <rtc.h>

#define LCD_CON_OCR OCR0A
#define LCD_BKL_OCR OCR0B

lcd_t lcd = {
  .interface = LCD_INTERFACE_8BIT,
  .data = {&PINA, &PORTA, &DDRA, PA0},
  .rs   = {&PINB, &PORTB, &DDRB, PB1},
  .rw   = {&PINB, &PORTB, &DDRB, PB2},
  .e    = {&PINB, &PORTB, &DDRB, PB0}
};

#define MAX_DATA_AGE_CS 150

struct {
  rtc_datetime_24h_t current_dt;
  uint8_t gps_signal_strength;
} data;
uint8_t *data_p;
uint8_t data_age = MAX_DATA_AGE_CS;

#define CG_GPS_ICON 0
lcd_cg_t cg_gps_icon = {
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000110,
  0b00001000,
  0b00001011,
  0b00001001,
  0b00000110,
};

#define CG_GPS_SIGNAL 1
#define CG_GPS_SIGNAL_SIZE 9
lcd_cg_t cg_gps_signal[9] = {
  { // 0, "X" drawing
    0b00000000,
    0b00000000,
    0b00000000,
    0b00010001,
    0b00001010,
    0b00000100,
    0b00001010,
    0b00010001,
  },
  { // 1
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000001,
  },
  { // 2
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000001,
    0b00000011,
  },
  { // 3
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000001,
    0b00000011,
    0b00000111,
  },
  { // 4
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000001,
    0b00000011,
    0b00000111,
    0b00001111,
  },
  { // 5
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000001,
    0b00000011,
    0b00000111,
    0b00001111,
    0b00011111,
  },
  { // 6
    0b00000000,
    0b00000000,
    0b00000001,
    0b00000011,
    0b00000111,
    0b00001111,
    0b00011111,
    0b00011111,
  },
  { // 7
    0b00000000,
    0b00000001,
    0b00000011,
    0b00000111,
    0b00001111,
    0b00011111,
    0b00011111,
    0b00011111,
  },
  { // 8
    0b00000001,
    0b00000011,
    0b00000111,
    0b00001111,
    0b00011111,
    0b00011111,
    0b00011111,
    0b00011111,
  },
};

uint8_t handle_i2c_slave_rx(uint8_t status, i2c_mode_t last_mode, i2c_mode_t current_mode)
{
  if(last_mode != current_mode)
  {
    data_p = (uint8_t *)&data;
  }

  if(status == TW_SR_DATA_ACK || status == TW_SR_DATA_NACK)
    *data_p++ = TWDR;

  data_age = 0;

  return 0;
}

int main(void)
{
  uart_t *u0;
  char s[32];

  _delay_ms(1000);

  u0 = uart_init("0", UART_BAUD_SELECT(38400, F_CPU));
  uart_init_stdout(u0);

  i2c_init();
  i2c_slave_init(0x70, I2C_ADDRESS_MASK_SINGLE, I2C_GCALL_DISABLED);
  i2c_global.sr_callback = handle_i2c_slave_rx;

  DDRB |= _BV(PB3) | _BV(PB4);

  /*
    COM0A1 = Clear OC0A on compare match
    COM0B1 = Clear OC0B on compare match
    WGM01|WGM00 = Fast PWM
  */
  TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
  /* CS01 = Prescale by /8 */
  TCCR0B = _BV(CS01);

  LCD_CON_OCR = 90;
  LCD_BKL_OCR = 255;

  sei();

  printf("\n\nBooted up!\n");

  lcd_init(&lcd);

  lcd_cg_define(&lcd, CG_GPS_ICON, cg_gps_icon);
  lcd_cg_define(&lcd, CG_GPS_SIGNAL, cg_gps_signal[0]);

  while(1)
  {
    if(data_age == 0)
    {
      // Re-define the CG_GPS_SIGNAL character to a depiction of the current
      // signal strength (an "X" for no signal, or a number of bars).
      lcd_cg_define(&lcd, CG_GPS_SIGNAL,
          cg_gps_signal[data.gps_signal_strength]);

      sprintf(s, "%-10s%-4s%2i",
        rtc_dow_names[data.current_dt.day_of_week],
        rtc_month_abbreviations[data.current_dt.month],
        data.current_dt.date);
      lcd_move_cursor(&lcd, 0, 0);
      lcd_write_string(&lcd, s, strlen(s));

      sprintf(s, "    %02i:%02i:%02i    ",
        data.current_dt.hour,
        data.current_dt.minute,
        data.current_dt.second);
      lcd_move_cursor(&lcd, 1, 0);
      lcd_write_string(&lcd, s, strlen(s));

      // Draw the "G" icon.
      lcd_move_cursor(&lcd, 1, 14);
      lcd_write_char(&lcd, CG_GPS_ICON);

      // Draw the previously defined signal strength icon.
      lcd_move_cursor(&lcd, 1, 15);
      lcd_write_char(&lcd, CG_GPS_SIGNAL);

      // Between 9pm and 6am, dim the display, otherwise maximum brightness.
      if(data.current_dt.hour >= 6 && data.current_dt.hour < 21)
        LCD_BKL_OCR = 255;
      else
        LCD_BKL_OCR = 64;

    } else if(data_age == MAX_DATA_AGE_CS) {
      // The last data we've received is too old to display. It's better to
      // display an error than an incorrect time.
      LCD_BKL_OCR = 255;
      lcd_clear(&lcd);
      lcd_move_cursor(&lcd, 0, 0);
      lcd_write_string(&lcd, "Waiting for", 11);
      lcd_move_cursor(&lcd, 1, 5);
      lcd_write_string(&lcd, "valid data!", 11);
    }

    if(data_age <= MAX_DATA_AGE_CS) {
      data_age++;
    }

    _delay_ms(10);
  }

  return(0);
}
