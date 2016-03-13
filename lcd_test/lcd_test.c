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
#include <lcd.h>

#define LCD_CON_OCR OCR0A
#define LCD_BKL_OCR OCR0B

lcd_t lcd = {
  .interface = LCD_INTERFACE_8BIT,
  .data = {&PINA, &PORTA, &DDRA, PA0},
  .rs   = {&PINB, &PORTB, &DDRB, PB1},
  .rw   = {&PINB, &PORTB, &DDRB, PB2},
  .e    = {&PINB, &PORTB, &DDRB, PB0}
};

int main(void)
{
  uart_t *u0;
  char s[32];
  uint16_t x = 0;

  _delay_ms(1000);

  u0 = uart_init("0", UART_BAUD_SELECT(38400, F_CPU));
  uart_init_stdout(u0);

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

  while(1)
  {
    if(0 == (x%26))
      lcd_clear(&lcd);

    sprintf(s, "%-11s%5i", "Test String", x);
    lcd_move_cursor(&lcd, 0, 0);
    lcd_write_string(&lcd, s, strlen(s));

    lcd_move_cursor(&lcd, 1, x%16);
    lcd_write_char(&lcd, x%26+'a');

    _delay_ms(200);
    x++;
  }

  return(0);
}
