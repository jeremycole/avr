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

#include <stdlib.h>
#include <string.h>
#include <avr/io.h>

#include "led_charlieplex.h"

void led_charlieplex_init(led_charlieplex_t *charlieplex)
{
  led_charlieplex_port_t *port = charlieplex->port;

  for(; port->pins != 0x00; port++)
  {
    *(port->ddr)  &= ~(port->pins);
    *(port->port) &= ~(port->pins);
  }
}

uint8_t led_charlieplex_find_index_by_name(led_charlieplex_t *charlieplex, char *name)
{
  led_charlieplex_led_t *led = charlieplex->led;
  uint8_t index = 0;

  for(; led->name != NULL; led++, index++)
  {
    if(strcasecmp(led->name, name) == 0)
      return index;
  }

  return LED_CHARLIEPLEX_LED_UNKNOWN;
}

inline void led_charlieplex_unset(led_charlieplex_t *charlieplex, led_charlieplex_led_t *led)
{
  *(charlieplex->port[led->h_port].ddr) &= ~_BV(led->h_value);
  *(charlieplex->port[led->l_port].ddr) &= ~_BV(led->l_value);

  *(charlieplex->port[led->h_port].port) &= ~_BV(led->h_value);
  *(charlieplex->port[led->l_port].port) &= ~_BV(led->l_value);
}

inline void led_charlieplex_unset_last_inline(led_charlieplex_t *charlieplex)
{
  if(charlieplex->last_led)
  {
    led_charlieplex_unset(charlieplex, charlieplex->last_led);
    charlieplex->last_led = NULL;
  }
}

void led_charlieplex_unset_last(led_charlieplex_t *charlieplex)
{
  led_charlieplex_unset_last_inline(charlieplex);
}

inline void led_charlieplex_set(led_charlieplex_t *charlieplex, led_charlieplex_led_t *led)
{
  led_charlieplex_unset_last_inline(charlieplex);

  /*
   * Set the data direction registers so that unused pins for this LED
   * are not set as outputs.  This will ensure that the pullups and
   * pulldowns for that pin are disabled
   */
  *(charlieplex->port[led->h_port].ddr) |= _BV(led->h_value);
  *(charlieplex->port[led->l_port].ddr) |= _BV(led->l_value);

  /*
   * Enable the pullup on the high side of the LED and enable the pulldown
   * on the low side.
   */
  *(charlieplex->port[led->h_port].port) |= _BV(led->h_value);
  *(charlieplex->port[led->l_port].port) &= ~_BV(led->l_value);

  /* Track the LED set so that it can be quickly unset later. */
  charlieplex->last_led = led;
}

void led_charlieplex_set_by_name(led_charlieplex_t *charlieplex, char *name)
{
  uint8_t led_index;

  led_index = led_charlieplex_find_index_by_name(charlieplex, name);

  if(led_index == LED_CHARLIEPLEX_LED_UNKNOWN)
    return;

  led_charlieplex_set(charlieplex, &charlieplex->led[led_index]);
}

void led_charlieplex_set_by_index(led_charlieplex_t *charlieplex, uint8_t index)
{
  led_charlieplex_set(charlieplex, &charlieplex->led[index]);
}
