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

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "lcd.h"

void lcd_init(lcd_t *lcd)
{
  *lcd->rs.ddr |= _BV(lcd->rs.bv);
  *lcd->rw.ddr |= _BV(lcd->rw.bv);
  *lcd->e.ddr  |= _BV(lcd->e.bv);

  _delay_ms(20);
  lcd_parameters(lcd, lcd->interface);

  _delay_ms(6);
  lcd_parameters(lcd, lcd->interface);

  _delay_us(150);
  lcd_parameters(lcd, lcd->interface);

  lcd_parameters(lcd, lcd->interface | LCD_LINES_2 | LCD_FONT_5X8);
  lcd_entry_mode(lcd, LCD_CURSOR_INCREMENT | LCD_DISPLAY_SHIFT_OFF);
  lcd_clear(lcd);
  lcd_display_mode(lcd, LCD_DISPLAY_ON);
}

uint8_t _lcd_read_data(lcd_t *lcd)
{
  uint8_t data = 0;

  *lcd->e.port    |=  _BV(lcd->e.bv);
  _delay_us(0.1);
  switch(lcd->interface)
  {
  case LCD_INTERFACE_8BIT:
    data           = *lcd->data.pin;
    break;
  case LCD_INTERFACE_4BIT:
    data = ((*lcd->data.pin & (0x0f << lcd->data.bv)) >> lcd->data.bv) << 4;
    _delay_us(0.3);
    *lcd->e.port    &= ~_BV(lcd->e.bv);
    _delay_us(0.3);
    *lcd->e.port    |=  _BV(lcd->e.bv);
    _delay_us(0.3);
    data |= ((*lcd->data.pin & (0x0f << lcd->data.bv)) >> lcd->data.bv);
    break;
  }
  _delay_us(0.3);
  *lcd->e.port    &= ~_BV(lcd->e.bv);
  _delay_us(0.3);

  switch(lcd->interface)
  {
  case LCD_INTERFACE_8BIT:
    *lcd->data.port  = 0x00;
    *lcd->data.ddr   = 0x00;
    break;
  case LCD_INTERFACE_4BIT:
    *lcd->data.port  &= ~(0x0f << lcd->data.bv);
    *lcd->data.ddr   &= ~(0x0f << lcd->data.bv);
    break;
  }

  return data;
}

void _lcd_write_data(lcd_t *lcd, uint8_t data)
{
  *lcd->e.port    |=  _BV(lcd->e.bv);
  _delay_us(0.1);
  switch(lcd->interface)
  {
  case LCD_INTERFACE_8BIT:
    *lcd->data.ddr   = 0xff;
    *lcd->data.port  = data;
    break;
  case LCD_INTERFACE_4BIT:
    *lcd->data.ddr   |= 0x0f << lcd->data.bv;
    *lcd->data.port  |= ((data & 0xf0) >> 4) << lcd->data.bv;
    _delay_us(0.3);
    *lcd->e.port    &= ~_BV(lcd->e.bv);
    *lcd->data.port &= ~(0x0f << lcd->data.bv);
    _delay_us(0.3);
    *lcd->e.port    |=  _BV(lcd->e.bv);
    _delay_us(0.3);
    *lcd->data.port  |= (data & 0x0f) << lcd->data.bv;
    break;
  }
  _delay_us(0.3);
  *lcd->e.port    &= ~_BV(lcd->e.bv);
  _delay_us(0.3);

  switch(lcd->interface)
  {
  case LCD_INTERFACE_8BIT:
    *lcd->data.port  = 0x00;
    *lcd->data.ddr   = 0x00;
    break;
  case LCD_INTERFACE_4BIT:
    *lcd->data.port  &= ~(0x0f << lcd->data.bv);
    *lcd->data.ddr   &= ~(0x0f << lcd->data.bv);
    break;
  }
}

uint8_t lcd_read_sr(lcd_t *lcd)
{
  int status;

  *lcd->rw.port   |=  _BV(lcd->rw.bv);
  *lcd->rs.port   &= ~_BV(lcd->rs.bv);
  _delay_us(0.1);
  status           = _lcd_read_data(lcd);
  *lcd->rs.port   &= ~_BV(lcd->rs.bv);
  *lcd->rw.port   &= ~_BV(lcd->rw.bv);
  _delay_us(0.2);

  return status;
}

void lcd_write_ir(lcd_t *lcd, uint8_t instruction)
{
  while ((lcd_read_sr(lcd) & LCD_BUSY) != 0) {};

  *lcd->rs.port   &= ~_BV(lcd->rs.bv);
  *lcd->rw.port   &= ~_BV(lcd->rw.bv);
  _delay_us(0.1);
  _lcd_write_data(lcd, instruction);
  _delay_us(0.2);
}

void lcd_write_dr(lcd_t *lcd, uint8_t data)
{
  while ((lcd_read_sr(lcd) & LCD_BUSY) != 0) {};

  *lcd->rw.port   &= ~_BV(lcd->rw.bv);
  *lcd->rs.port   |=  _BV(lcd->rs.bv);
  _delay_us(0.1);
  _lcd_write_data(lcd, data);
  *lcd->rs.port   &= ~_BV(lcd->rs.bv);
  *lcd->rw.port   &= ~_BV(lcd->rw.bv);
  _delay_us(0.2);
}

void lcd_display_mode(lcd_t *lcd, uint8_t mode)
{
  lcd_write_ir(lcd, LCD_IR_MODE_DISPLAY | mode);
}

void lcd_parameters(lcd_t *lcd, uint8_t parameters)
{
  lcd_write_ir(lcd, LCD_IR_MODE_PARAMETERS | parameters);
}

void lcd_entry_mode(lcd_t *lcd, uint8_t mode)
{
  lcd_write_ir(lcd, LCD_IR_MODE_ENTRY | mode);
}

void lcd_move_cursor(lcd_t *lcd, uint8_t line, uint8_t col)
{
  lcd_write_ir(lcd, LCD_IR_MODE_DDRAM_ADDR | ((LCD_LINE_DDRAM_WIDTH * line) + col) );
}

void lcd_cg_define(lcd_t *lcd, uint8_t character, uint8_t data[8])
{
  uint8_t pos;
  lcd_write_ir(lcd, LCD_IR_MODE_CGRAM_ADDR | (character * 8));
  for(pos=0; pos < 8; pos++)
  {
    lcd_write_dr(lcd, data[pos]);
  }
}

void lcd_clear(lcd_t *lcd)
{
  lcd_write_ir(lcd, LCD_IR_CLEAR);
}

void lcd_return_home(lcd_t *lcd)
{
  lcd_write_ir(lcd, LCD_IR_RETURN_HOME);
}

void lcd_write_char(lcd_t *lcd, char c)
{
  lcd_write_dr(lcd, c);
}

void lcd_write_string(lcd_t *lcd, char *s, int n)
{
  for (; n>0; n--, s++)
  {
    lcd_write_dr(lcd, *s);
  }
}
