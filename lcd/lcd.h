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

#ifndef LCD_H
#define LCD_H

#include <avr/io.h>

#define LCD_IR_CLEAR           (0x01)
#define LCD_IR_RETURN_HOME     (0x02)

#define LCD_IR_MODE_ENTRY      (0x04)
#define LCD_CURSOR_DECREMENT   (0x00)
#define LCD_CURSOR_INCREMENT   (0x02)
#define LCD_DISPLAY_SHIFT_OFF  (0x00)
#define LCD_DISPLAY_SHIFT_ON   (0x01)

#define LCD_IR_MODE_DISPLAY    (0x08)
#define LCD_DISPLAY_ON         (0x04)
#define LCD_CURSOR_ON          (0x02)
#define LCD_CURSOR_BLINK       (0x01)

#define LCD_IR_MODE_PARAMETERS (0x20)
#define LCD_INTERFACE_4BIT     (0x00)
#define LCD_INTERFACE_8BIT     (0x10)
#define LCD_LINES_1            (0x00)
#define LCD_LINES_2            (0x08)
#define LCD_FONT_5X8           (0x00)
#define LCD_FONT_5X11          (0x04)

#define LCD_IR_MODE_CGRAM_ADDR (0x40)
#define LCD_IR_MODE_DDRAM_ADDR (0x80)

#define LCD_BUSY               (0x80)

#define LCD_LINE_DDRAM_WIDTH   (0x40)

typedef struct _lcd_port_t
{
  volatile uint8_t *pin;
  volatile uint8_t *port;
  volatile uint8_t *ddr;
  uint8_t bv;
} lcd_port_t;

typedef struct _lcd_t
{
  uint8_t interface;
  lcd_port_t data;
  lcd_port_t rs;
  lcd_port_t rw;
  lcd_port_t e;
} lcd_t;

typedef uint8_t lcd_cg_t[8];

void lcd_init(lcd_t *lcd);
uint8_t lcd_read_sr(lcd_t *lcd);
void lcd_write_ir(lcd_t *lcd, uint8_t instruction);
void lcd_write_dr(lcd_t *lcd, uint8_t data);
void lcd_display_mode(lcd_t *lcd, uint8_t mode);
void lcd_parameters(lcd_t *lcd, uint8_t parameters);
void lcd_entry_mode(lcd_t *lcd, uint8_t mode);
void lcd_move_cursor(lcd_t *lcd, uint8_t line, uint8_t col);
void lcd_cg_define(lcd_t *lcd, uint8_t character, uint8_t data[8]);
void lcd_clear(lcd_t *lcd);
void lcd_return_home(lcd_t *lcd);
void lcd_write_char(lcd_t *lcd, char c);
void lcd_write_string(lcd_t *lcd, char *s, int n);

#endif /* LCD_H */
