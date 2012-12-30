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
 * @file
 *
 * Mapping tables of all LEDs into logical positions.
 */

/**
 * Map of 60 LEDs in outer ring for minutes
 * A{A:H}1, B{A:G}1, C{A:H}1, D{A:G}1, E{A:H}1, F{A:G}1, G{A:H}1, H{A:G}1
 */
char *led_mapping_minute[60] = {
  "AA1", /* 00 */
  "AB1", /* 01 */
  "AC1", /* 02 */
  "AD1", /* 03 */
  "AE1", /* 04 */
  "AF1", /* 05 */
  "AG1", /* 06 */
  "AH1", /* 07 */
  "BA1", /* 08 */
  "BB1", /* 09 */
  "BC1", /* 10 */
  "BD1", /* 11 */
  "BE1", /* 12 */
  "BF1", /* 13 */
  "BG1", /* 14 */

  "CA1", /* 15 */
  "CB1", /* 16 */
  "CC1", /* 17 */
  "CD1", /* 18 */
  "CE1", /* 19 */
  "CF1", /* 20 */
  "CG1", /* 21 */
  "CH1", /* 22 */
  "DA1", /* 23 */
  "DB1", /* 24 */
  "DC1", /* 25 */
  "DD1", /* 26 */
  "DE1", /* 27 */
  "DF1", /* 28 */
  "DG1", /* 29 */

  "EA1", /* 30 */
  "EB1", /* 31 */
  "EC1", /* 32 */
  "ED1", /* 33 */
  "EE1", /* 34 */
  "EF1", /* 35 */
  "EG1", /* 36 */
  "EH1", /* 37 */
  "FA1", /* 38 */
  "FB1", /* 39 */
  "FC1", /* 40 */
  "FD1", /* 41 */
  "FE1", /* 42 */
  "FF1", /* 43 */
  "FG1", /* 44 */

  "GA1", /* 45 */
  "GB1", /* 46 */
  "GC1", /* 47 */
  "GD1", /* 48 */
  "GE1", /* 49 */
  "GF1", /* 50 */
  "GG1", /* 51 */
  "GH1", /* 52 */
  "HA1", /* 53 */
  "HB1", /* 54 */
  "HC1", /* 55 */
  "HD1", /* 56 */
  "HE1", /* 57 */
  "HF1", /* 58 */
  "HG1", /* 59 */
};

/**
 * Map of 48 LEDs in inner ring for quarter hours
 * A{A:H}2, B{A:D}2, C{A:H}2, D{A:D}2, E{A:H}2, F{A:D}2, G{A:H}2, H{A:D}2
 */
char *led_mapping_qhour[48] = {
  "AA2", /* 00 */
  "AB2", /* 01 */
  "AC2", /* 02 */
  "AD2", /* 03 */

  "AE2", /* 04 */
  "AF2", /* 05 */
  "AG2", /* 06 */
  "AH2", /* 07 */

  "BA2", /* 08 */
  "BB2", /* 09 */
  "BC2", /* 10 */
  "BD2", /* 11 */

  "CA2", /* 12 */
  "CB2", /* 13 */
  "CC2", /* 14 */
  "CD2", /* 15 */

  "CE2", /* 16 */
  "CF2", /* 17 */
  "CG2", /* 18 */
  "CH2", /* 19 */

  "DA2", /* 20 */
  "DB2", /* 21 */
  "DC2", /* 22 */
  "DD2", /* 23 */

  "EA2", /* 24 */
  "EB2", /* 25 */
  "EC2", /* 26 */
  "ED2", /* 27 */

  "EE2", /* 28 */
  "EF2", /* 29 */
  "EG2", /* 30 */
  "EH2", /* 31 */

  "FA2", /* 32 */
  "FB2", /* 33 */
  "FC2", /* 34 */
  "FD2", /* 35 */

  "GA2", /* 36 */
  "GB2", /* 37 */
  "GC2", /* 38 */
  "GD2", /* 39 */

  "GE2", /* 40 */
  "GF2", /* 41 */
  "GG2", /* 42 */
  "GH2", /* 43 */

  "HA2", /* 44 */
  "HB2", /* 45 */
  "HC2", /* 46 */
  "HD2", /* 47 */
};

/**
 * Map of 5 LEDs in bar at 0 degrees
 * B{E:H}2, BH1
 */
char *led_mapping_bar000[5] = {
  "BE2", /* 00 */
  "BF2", /* 01 */
  "BG2", /* 02 */
  "BH2", /* 03 */
  "BH1" /* 04 */
};

/**
 * Map of 5 LEDs in bar at 90 degrees
 * D{E:H}2, DH1
 */
char *led_mapping_bar090[5] = {
  "DE2", /* 00 */
  "DF2", /* 01 */
  "DG2", /* 02 */
  "DH2", /* 03 */
  "DH1" /* 04 */
};

/**
 * Map of 5 LEDs in bar at 180 degrees
 * F{E:H}2, FH1
 */
char *led_mapping_bar180[5] = {
  "FE2", /* 00 */
  "FF2", /* 01 */
  "FG2", /* 02 */
  "FH2", /* 03 */
  "FH1" /* 04 */
};

/**
 * Map of 5 LEDs in bar at 270 degrees
 * H{E:H}2, HH1
 */
char *led_mapping_bar270[5] = {
  "HE2", /* 00 */
  "HF2", /* 01 */
  "HG2", /* 02 */
  "HH2", /* 03 */
  "HH1", /* 04 */
};

/**
 * Map of all bars, iterating across
 */
char *led_mapping_across_all_bars[20] = {
  "BE2", /* 00 */
  "BF2", /* 01 */
  "BG2", /* 02 */
  "BH2", /* 03 */
  "BH1", /* 04 */

  "DE2", /* 00 */
  "DF2", /* 01 */
  "DG2", /* 02 */
  "DH2", /* 03 */
  "DH1", /* 04 */

  "FE2", /* 00 */
  "FF2", /* 01 */
  "FG2", /* 02 */
  "FH2", /* 03 */
  "FH1", /* 04 */

  "HE2", /* 00 */
  "HF2", /* 01 */
  "HG2", /* 02 */
  "HH2", /* 03 */
  "HH1", /* 04 */
};

/**
 * Map of all bars, iterating around
 */
char *led_mapping_around_all_bars[20] = {
  "BE2", /* 00 */
  "DE2", /* 00 */
  "FE2", /* 00 */
  "HE2", /* 00 */

  "BF2", /* 01 */
  "DF2", /* 01 */
  "FF2", /* 01 */
  "HF2", /* 01 */

  "BG2", /* 02 */
  "DG2", /* 02 */
  "FG2", /* 02 */
  "HG2", /* 02 */

  "BH2", /* 03 */
  "DH2", /* 03 */
  "FH2", /* 03 */
  "HH2", /* 03 */

  "BH1", /* 04 */
  "DH1", /* 04 */
  "FH1", /* 04 */
  "HH1", /* 04 */
};

/**
 * Innermost 4 LEDs in bars
 */
char *led_mapping_inner[4] = {
  "BH1",
  "DH1",
  "FH1",
  "HH1",
};
