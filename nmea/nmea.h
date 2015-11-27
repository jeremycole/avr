/*
    Copyright (c) 2015, Jeremy Cole <jeremy@jcole.us>

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

#ifndef NMEA_H_
#define NMEA_H_

#include <inttypes.h>

#define NMEA_EPOCH 80

#define NMEA_INVALID_TYPE           0x0001
#define NMEA_INVALID_SENTENCE       0x0002
#define NMEA_INVALID_TIME           0x0004
#define NMEA_INVALID_DATE           0x0008
#define NMEA_INVALID_LATITUDE       0x0010
#define NMEA_INVALID_LONGITUDE      0x0020
#define NMEA_INVALID_UNIT           0x2000
#define NMEA_INVALID_STATUS         0x4000
#define NMEA_INVALID_CHECKSUM       0x8000

typedef struct _nmea_date_t
{
  uint16_t year;
  uint8_t month;
  uint8_t day;
} nmea_date_t;

typedef struct _nmea_time_t
{
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint16_t millisecond;
} nmea_time_t;

typedef struct _nmea_position_t
{
  float latitude;
  float longitude;
} nmea_position_t;

typedef struct _nmea_velocity_t
{
  float speed;
  float heading;
} nmea_velocity_t;

typedef struct _nmea_gprmc_t
{
  char status;
  char mode;
  nmea_date_t date;
  nmea_time_t time;
  nmea_position_t position;
  nmea_velocity_t velocity;
  char checksum;
} nmea_gprmc_t;

typedef struct _nmea_gpgga_t
{
  nmea_time_t time;
  nmea_position_t position;
  uint8_t fix_quality;
  uint8_t satellites_tracked;
  float hdop;
  float altitude;
  float geoid_height;
  char checksum;
} nmea_gpgga_t;

typedef struct _nmea_gpgsa_t
{
  char mode;
  char fix_type;
  uint8_t satellite_prn[12];
  float pdop;
  float hdop;
  float vdop;
  char checksum;
} nmea_gpgsa_t;

typedef struct _nmea_gpgsv_satellite_t
{
  uint8_t index;
  uint8_t prn;
  uint8_t altitude;
  uint16_t azimuth;
  uint8_t snr;
} nmea_gpgsv_satellite_t;

typedef struct _nmea_gpgsv_t
{
  uint8_t sentence_total;
  uint8_t sentence_number;
  uint8_t satellites_in_view;
  nmea_gpgsv_satellite_t satellite[4];
  char checksum;
} nmea_gpgsv_t;

extern unsigned int nmea_parse_gprmc(char *sentence, nmea_gprmc_t *data);
extern unsigned int nmea_parse_gpgga(char *sentence, nmea_gpgga_t *data);
extern unsigned int nmea_parse_gpgsa(char *sentence, nmea_gpgsa_t *data);
extern unsigned int nmea_parse_gpgsv(char *sentence, nmea_gpgsv_t *data);

#endif /* NMEA_H_ */
