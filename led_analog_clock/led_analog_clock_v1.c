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
#include <avr/io.h>

#include <led_charlieplex.h>

#include "led_analog_clock_v1.h"

led_charlieplex_port_t led_analog_clock_v1_ports[] = {
  /* 0 */ { &PINA, &DDRA, &PORTA, 0xff },
  /* 1 */ { &PINC, &DDRC, &PORTC, 0xff },
  { NULL, NULL, NULL, 0 },
};

led_charlieplex_led_t led_analog_clock_v1_leds[] = {
  { "AA1", 0, 0, 1, 0 },
  { "AB1", 0, 0, 1, 1 },
  { "AC1", 0, 0, 1, 2 },
  { "AD1", 0, 0, 1, 3 },
  { "AE1", 0, 0, 1, 4 },
  { "AF1", 0, 0, 1, 5 },
  { "AG1", 0, 0, 1, 6 },
  { "AH1", 0, 0, 1, 7 },

  { "AA2", 1, 0, 0, 0 },
  { "AB2", 1, 1, 0, 0 },
  { "AC2", 1, 2, 0, 0 },
  { "AD2", 1, 3, 0, 0 },
  { "AE2", 1, 4, 0, 0 },
  { "AF2", 1, 5, 0, 0 },
  { "AG2", 1, 6, 0, 0 },
  { "AH2", 1, 7, 0, 0 },

  { "BA1", 0, 1, 1, 0 },
  { "BB1", 0, 1, 1, 1 },
  { "BC1", 0, 1, 1, 2 },
  { "BD1", 0, 1, 1, 3 },
  { "BE1", 0, 1, 1, 4 },
  { "BF1", 0, 1, 1, 5 },
  { "BG1", 0, 1, 1, 6 },
  { "BH1", 0, 1, 1, 7 },

  { "BA2", 1, 0, 0, 1 },
  { "BB2", 1, 1, 0, 1 },
  { "BC2", 1, 2, 0, 1 },
  { "BD2", 1, 3, 0, 1 },
  { "BE2", 1, 4, 0, 1 },
  { "BF2", 1, 5, 0, 1 },
  { "BG2", 1, 6, 0, 1 },
  { "BH2", 1, 7, 0, 1 },

  { "CA1", 0, 2, 1, 0 },
  { "CB1", 0, 2, 1, 1 },
  { "CC1", 0, 2, 1, 2 },
  { "CD1", 0, 2, 1, 3 },
  { "CE1", 0, 2, 1, 4 },
  { "CF1", 0, 2, 1, 5 },
  { "CG1", 0, 2, 1, 6 },
  { "CH1", 0, 2, 1, 7 },

  { "CA2", 1, 0, 0, 2 },
  { "CB2", 1, 1, 0, 2 },
  { "CC2", 1, 2, 0, 2 },
  { "CD2", 1, 3, 0, 2 },
  { "CE2", 1, 4, 0, 2 },
  { "CF2", 1, 5, 0, 2 },
  { "CG2", 1, 6, 0, 2 },
  { "CH2", 1, 7, 0, 2 },

  { "DA1", 0, 3, 1, 0 },
  { "DB1", 0, 3, 1, 1 },
  { "DC1", 0, 3, 1, 2 },
  { "DD1", 0, 3, 1, 3 },
  { "DE1", 0, 3, 1, 4 },
  { "DF1", 0, 3, 1, 5 },
  { "DG1", 0, 3, 1, 6 },
  { "DH1", 0, 3, 1, 7 },

  { "DA2", 1, 0, 0, 3 },
  { "DB2", 1, 1, 0, 3 },
  { "DC2", 1, 2, 0, 3 },
  { "DD2", 1, 3, 0, 3 },
  { "DE2", 1, 4, 0, 3 },
  { "DF2", 1, 5, 0, 3 },
  { "DG2", 1, 6, 0, 3 },
  { "DH2", 1, 7, 0, 3 },

  { "EA1", 0, 4, 1, 0 },
  { "EB1", 0, 4, 1, 1 },
  { "EC1", 0, 4, 1, 2 },
  { "ED1", 0, 4, 1, 3 },
  { "EE1", 0, 4, 1, 4 },
  { "EF1", 0, 4, 1, 5 },
  { "EG1", 0, 4, 1, 6 },
  { "EH1", 0, 4, 1, 7 },

  { "EA2", 1, 0, 0, 4 },
  { "EB2", 1, 1, 0, 4 },
  { "EC2", 1, 2, 0, 4 },
  { "ED2", 1, 3, 0, 4 },
  { "EE2", 1, 4, 0, 4 },
  { "EF2", 1, 5, 0, 4 },
  { "EG2", 1, 6, 0, 4 },
  { "EH2", 1, 7, 0, 4 },

  { "FA1", 0, 5, 1, 0 },
  { "FB1", 0, 5, 1, 1 },
  { "FC1", 0, 5, 1, 2 },
  { "FD1", 0, 5, 1, 3 },
  { "FE1", 0, 5, 1, 4 },
  { "FF1", 0, 5, 1, 5 },
  { "FG1", 0, 5, 1, 6 },
  { "FH1", 0, 5, 1, 7 },

  { "FA2", 1, 0, 0, 5 },
  { "FB2", 1, 1, 0, 5 },
  { "FC2", 1, 2, 0, 5 },
  { "FD2", 1, 3, 0, 5 },
  { "FE2", 1, 4, 0, 5 },
  { "FF2", 1, 5, 0, 5 },
  { "FG2", 1, 6, 0, 5 },
  { "FH2", 1, 7, 0, 5 },

  { "GA1", 0, 6, 1, 0 },
  { "GB1", 0, 6, 1, 1 },
  { "GC1", 0, 6, 1, 2 },
  { "GD1", 0, 6, 1, 3 },
  { "GE1", 0, 6, 1, 4 },
  { "GF1", 0, 6, 1, 5 },
  { "GG1", 0, 6, 1, 6 },
  { "GH1", 0, 6, 1, 7 },

  { "GA2", 1, 0, 0, 6 },
  { "GB2", 1, 1, 0, 6 },
  { "GC2", 1, 2, 0, 6 },
  { "GD2", 1, 3, 0, 6 },
  { "GE2", 1, 4, 0, 6 },
  { "GF2", 1, 5, 0, 6 },
  { "GG2", 1, 6, 0, 6 },
  { "GH2", 1, 7, 0, 6 },

  { "HA1", 0, 7, 1, 0 },
  { "HB1", 0, 7, 1, 1 },
  { "HC1", 0, 7, 1, 2 },
  { "HD1", 0, 7, 1, 3 },
  { "HE1", 0, 7, 1, 4 },
  { "HF1", 0, 7, 1, 5 },
  { "HG1", 0, 7, 1, 6 },
  { "HH1", 0, 7, 1, 7 },

  { "HA2", 1, 0, 0, 7 },
  { "HB2", 1, 1, 0, 7 },
  { "HC2", 1, 2, 0, 7 },
  { "HD2", 1, 3, 0, 7 },
  { "HE2", 1, 4, 0, 7 },
  { "HF2", 1, 5, 0, 7 },
  { "HG2", 1, 6, 0, 7 },
  { "HH2", 1, 7, 0, 7 },

  { NULL, 0, 0, 0, 0 }
};

led_charlieplex_t led_analog_clock_v1 = {
  0,
  led_analog_clock_v1_ports,
  led_analog_clock_v1_leds
};

