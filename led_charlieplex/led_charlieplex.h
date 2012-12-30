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

/**

@mainpage

A library to handle a charlieplexed matrix of LEDs.

@section about About Charliplexing

Charlieplexing is a technique used to allow a large number of LEDs to be
used in a matrix display, while only using a small number of IO pins.  It
depends on tri-state logic to allow pins to "float".

A charlieplexed display is arranged in an X-Y matrix of cells as typical,
but each cell contains two LEDs in opposite polarities as follows:

@verbatim
  X      Y
  |      |
  +-->|--+  (a)
  +--|<--+  (b)
@endverbatim

Due to LEDs being polarized one pin is pulled high and the other is pulled
low, one of the two LEDs will light.  When these cells are used in this way,
a 4x4 matrix could be constructed as follows:

@verbatim
        X0   X1   X2   X3
       ---- ---- ---- ----
  Y0 | x0y0 x1y0 x2y0 x3y0
  Y1 | x0y1 x1y1 x2y1 x3y1
  Y2 | x0y2 x1y2 x2y2 x3y2
  Y3 | x0y3 x1y3 x2y3 x3y3
@endverbatim

If X0 is pulled high and Y0 is pulled low while X{1-3} and Y{1-3} are allowed
to float (tri-stated), only x0y0a will light.  In order to minimize ghosting
and other display artifacts, only one LED can be lit at a time.

If a high enough refresh rate can be maintained, a charlieplexed display (as
any other LED display) can appear as though several LEDs are lit simultaneously.

More information can be found on Wikipedia:

  http://en.wikipedia.org/wiki/Charlieplexing

@section usage Use of this library

Typically you will want to populate a led_charlieplex_t structure
representing your charlieplexed matrix.  Usually it is easiest to do this
in three steps by creating and populating the sub-structures first.  For
example, using the previous 4x4 matrix, with:

@li \c X{0-3} connected to \c PB{0-3}
@li \c Y{0-3} connected to \c PD{4-7}

The led_charlieplex_port_t would be populated as follows:

@code
led_charlieplex_port_t led_example_ports[] = {
  { &PINB, &DDRB, &PORTB, (_BV(PB0) | _BV(PB1) | _BV(PB2) | _BV(PB3)) },
  { &PIND, &DDRD, &PORTD, (_BV(PD4) | _BV(PD5) | _BV(PD6) | _BV(PD7)) },
  { NULL, NULL, NULL, 0 },
};
@endcode

The led_charlieplex_led_t would be populated as follows:

@code
led_charlieplex_led_t led_example_leds[] = {
  { "x0y0a", 0, 0, 1, 4 },
  { "x1y0a", 0, 1, 1, 4 },
  { "x2y0a", 0, 2, 1, 4 },
  { "x3y0a", 0, 3, 1, 4 },

  { "x0y0b", 1, 4, 0, 0 },
  { "x1y0b", 1, 4, 0, 1 },
  { "x2y0b", 1, 4, 0, 2 },
  { "x3y0b", 1, 4, 0, 3 },

  ...

  { "x0y1a", 0, 0, 1, 5 },
  { "x1y1a", 0, 1, 1, 5 },
  { "x2y1a", 0, 2, 1, 5 },
  { "x3y1a", 0, 3, 1, 5 },

  { "x0y1b", 1, 5, 0, 0 },
  { "x1y1b", 1, 5, 0, 1 },
  { "x2y1b", 1, 5, 0, 2 },
  { "x3y1b", 1, 5, 0, 3 },

  ...

  { NULL, 0, 0, 0, 0 }
};
@endcode

And then the led_charlieplex_t would be populated by referencing both supporting
structures as follows:

@code
led_charlieplex_t led_example = {
  0,
  led_example_ports,
  led_example_leds
};
@endcode

*/

#ifndef LED_CHARLIEPLEX_H
#define LED_CHARLIEPLEX_H

/**
 * A special value meaning that no LED was found, returned where an index
 * would otherwise be returned.  This limits the maximum number of LEDs in a
 * matrix to 254, as index 255 is hereby reserved.
 */
#define LED_CHARLIEPLEX_LED_UNKNOWN 0xff

/**
 * A structure used to contain the PIN*, DDR*, PORT* and a bitmap of pins
 * in use on a single port used for this charlieplexed matrix.
 */
typedef struct _led_charlieplex_port_t
{
  volatile uint8_t *pin;    /**< The PIN* for this port, e.g. &PINC. */
  volatile uint8_t *ddr;    /**< The DDR* for this port, e.g. &DDRC. */
  volatile uint8_t *port;   /**< The PORT* for this port, e.g. &PORTC. */
  volatile uint8_t pins;    /**< The pins in use for this display, e.g. (_BV(PC0) & _BV(PC1)) or 0xFF. */
} led_charlieplex_port_t;

/**
 * A structure used to contain all pins name, and high and low pin information.
 *
 * The h_port and l_port members are indexes into the display's ->port array.
 *
 * The h_value and l_value members are the pin number (0-7) of the pin on the
 * associated port.  It is expanded later in all calculations as necessary.
 */
typedef struct _led_charlieplex_led_t
{
  char *name;               /**< The name of this LED, preferably short, e.g. "AA1". */
  uint8_t h_port:4;         /**< The high (+) port index, e.g. 0. */
  uint8_t h_value:4;        /**< The high (+) pin number, e.g. 1. */
  uint8_t l_port:4;         /**< The low (GND) port index, e.g. 1. */
  uint8_t l_value:4;        /**< The low (GND) pin number, e.g. 2. */
} led_charlieplex_led_t;

/**
 * A structure used to contain information about a charlieplexed matrix.
 */
typedef struct _led_charlieplex_t
{
  uint8_t dummy;

  /**
   * A NULL-terminated array of pointers to led_charlieplex_port_t structures
   * representing all of the ports in use for this charlieplexed matrix.
   */
  led_charlieplex_port_t *port;

  /**
   * A NULL-terminated array of pointers to led_charlieplex_led_t structures
   * representing the names and positions of all LEDs present in this charlie-
   * plexed matrix.
   */
  led_charlieplex_led_t *led;

  /**
   * (Internal use only.) A pointer to the last LED enabled, so that it can be
   * efficiently toggled off without affecting the rest of the matrix.
   */
  led_charlieplex_led_t *last_led;
} led_charlieplex_t;

/**
 * Initialize a charlieplexed matrix, clearing all LEDs.
 */
extern void led_charlieplex_init(led_charlieplex_t *charlieplex);

/**
 * Find the index into the led_charlieplex_t.led structure for led.
 *
 * @param[in]   charlieplex   A pointer to the led_charlieplex_t structure.
 * @param[in]   name          The name of the LED to search for.
 *
 * @return The index of the LED, if found, or LED_CHARLIEPLEX_LED_UNKNOWN if not found.
 */
extern uint8_t led_charlieplex_find_index_by_name(led_charlieplex_t *charlieplex, char *name);

/**
 * Set (enable) an LED by name.
 *
 * @param[in]   charlieplex   A pointer to the led_charlieplex_t structure.
 * @param[in]   name          The name of the LED to set.
 */
extern void led_charlieplex_set_by_name(led_charlieplex_t *charlieplex, char *name);

/**
 * Set (enable) an LED by index in the led_charlieplex_t.led array.
 *
 * @param[in]   charlieplex   A pointer to the led_charlieplex_t structure.
 * @param[in]   index         The index of the LED to set.
 */
extern void led_charlieplex_set_by_index(led_charlieplex_t *charlieplex, uint8_t index);

/**
 * Unset (disable) the last LED set, clearing the matrix.
 *
 * @param[in]   charlieplex   A pointer to the led_charlieplex_t structure.
 */
extern void led_charlieplex_unset_last(led_charlieplex_t *charlieplex);

#endif /* LED_CHARLIEPLEX_H */
