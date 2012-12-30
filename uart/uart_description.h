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
#ifndef UART_DESCRIPTION_H
#define UART_DESCRIPTION_H

#include <inttypes.h>
#include <avr/io.h>

typedef struct _uart_registers_t
{
  volatile uint8_t *status;
  volatile uint8_t *control;
  volatile uint8_t *format;
  volatile uint8_t *data;
  volatile uint8_t *baud_h;
  volatile uint8_t *baud_l;
} uart_registers_t;

typedef struct _uart_description_t
{
  char *name;
  uart_registers_t registers;
  uint8_t control_enable;
  uint8_t control_udrie;
  uint8_t status_u2x;
  uint8_t format_async;
  uint8_t format_8n1;
  uint8_t error_fe;
  uint8_t error_dor;
} uart_description_t;


extern uart_description_t uart_descriptions[];

#endif /* UART_DESCRIPTION_H */
