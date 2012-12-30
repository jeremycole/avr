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

#define MAX_DT_AGE_CS 150

rtc_datetime_24h_t current_dt;
uint8_t *current_dt_p;
uint8_t current_dt_age = MAX_DT_AGE_CS;

#define CG_HAPPY 0
uint8_t cg_happy[8] = {
  0b00000000,
  0b00000000,
  0b00001010,
  0b00000000,
  0b00010001,
  0b00001110,
  0b00000000,
  0b00000000,
};

#define CG_SAD 1
uint8_t cg_sad[8] = {
  0b00000000,
  0b00000000,
  0b00001010,
  0b00000000,
  0b00001110,
  0b00010001,
  0b00000000,
  0b00000000,
};


uint8_t handle_i2c_slave_rx(uint8_t status, i2c_mode_t last_mode, i2c_mode_t current_mode)
{
  if(last_mode != current_mode)
  {
    current_dt_p = (uint8_t *)&current_dt;
  }

  if(status == TW_SR_DATA_ACK || status == TW_SR_DATA_NACK)
    *current_dt_p++ = TWDR;

  current_dt_age = 0;

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

  lcd_cg_define(&lcd, CG_HAPPY, cg_happy);
  lcd_cg_define(&lcd, CG_SAD, cg_sad);

  while(1)
  {
    if(current_dt_age == 0)
    {
      sprintf(s, "%-10s%-4s%2i",
        rtc_dow_names[current_dt.day_of_week],
        rtc_month_abbreviations[current_dt.month],
        current_dt.date);
      lcd_move_cursor(&lcd, 0, 0);
      lcd_write_string(&lcd, s, strlen(s));

      sprintf(s, "    %02i:%02i:%02i    ",
        current_dt.hour, current_dt.minute, current_dt.second);
      lcd_move_cursor(&lcd, 1, 0);
      lcd_write_string(&lcd, s, strlen(s));

      lcd_move_cursor(&lcd, 1, 15);
      lcd_write_char(&lcd, CG_HAPPY);

      if(current_dt.hour >= 6 && current_dt.hour < 21)
        LCD_BKL_OCR = 255;
      else
        LCD_BKL_OCR = 64;

    } else if(current_dt_age == MAX_DT_AGE_CS) {
      LCD_BKL_OCR = 255;
      lcd_clear(&lcd);
      lcd_move_cursor(&lcd, 0, 0);
      lcd_write_string(&lcd, "Waiting for", 11);
      lcd_move_cursor(&lcd, 1, 5);
      lcd_write_string(&lcd, "valid data!", 11);
    }

    if(current_dt_age <= MAX_DT_AGE_CS) {
      current_dt_age++;
    }

    _delay_ms(10);
  }

  return(0);
}
