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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <avr/pgmspace.h>
#include "nmea.h"

static char *delim = ",*";

char nmea_checksum(char *s)
{
  char checksum = 0;
  if(*s++ != '$') return 0;
  for(; *s != '*' && *s != 0; s++)
  {
    checksum ^= *s;
  }
  return checksum;
}

unsigned int parse_char(char **fragment, char *out)
{
  char *token;

  if((token = strsep(fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token, PSTR("%c"), out);

  return 0;
}

unsigned int parse_uint8(char **fragment, uint8_t *out)
{
  char *token;

  if((token = strsep(fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token, PSTR("%hhu"), out);

  return 0;
}

unsigned int parse_uint16(char **fragment, uint16_t *out)
{
  char *token;

  if((token = strsep(fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token, PSTR("%hu"), out);

  return 0;
}

float extract_decimal(char *str, int sign)
{
  float out = 0.0;
  int place;
  for(place=strlen(str); place > 0; place--)
  {
    out += ((float)(str[place-1] - '0')) * pow(10, -place);
  }
  return out * (float)sign;
}

unsigned int parse_float(char **fragment, float *out)
{
  char *token;
  int16_t whole = 0;
  char frac[10] = {0,0,0,0,0,0,0,0,0,0};

  if((token = strsep(fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token, PSTR("%d.%s"), &whole, frac);
  *out = (float)whole + extract_decimal(frac, (whole<0 ? -1 : 1));

  return 0;
}

unsigned int parse_date(char **fragment, nmea_date_t *date)
{
  unsigned int invalidity = 0;
  char *token;

  /* Current UTC date */
  if((token = strsep(fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%02hhd%02hhd%02hd"),
    &date->day,
    &date->month,
    &date->year
  );

  /* Since 1980 is long past, if we see this year it's a good assumption
   * that the data is invalid.
   */
  if(date->year == 80)
    invalidity |= NMEA_INVALID_DATE;

  /* Adjust year based on GPS epoch of 1980 */
  date->year += (date->year) < 80 ? 2000 : 1900;

  return invalidity;
}

unsigned int parse_time(char **fragment, nmea_time_t *time)
{
  char *token;

  if((token = strsep(fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%02hhd%02hhd%02hhd.%03hd"),
    &time->hour,
    &time->minute,
    &time->second,
    &time->millisecond
  );

  return 0;
}

unsigned int parse_position(char **fragment, nmea_position_t *position)
{
  unsigned int invalidity = 0;
  char *token;
  char lat_hemisphere='N', lon_hemisphere='E';
  uint16_t degrees=0, minutes=0, minutes_frac=0;

  /* Latitude degrees and minutes */
  if((token = strsep(fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%02hd%02hd.%04hd"),
    &degrees,
    &minutes,
    &minutes_frac
  );

  position->latitude = (float)degrees + ((float)minutes / 60.0)
      + ((float)minutes_frac / 10000.0 / 60.0);

  /* Latitude hemisphere */
  if((token = strsep(fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%c"),
    &lat_hemisphere
  );

  degrees=0; minutes=0; minutes_frac=0;
  /* Longitude degrees and minutes */
  if((token = strsep(fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%03hd%02hd.%04hd"),
    &degrees,
    &minutes,
    &minutes_frac
  );

  position->longitude = (float)degrees + ((float)minutes / 60.0)
      + ((float)minutes_frac / 10000.0 / 60.0);

  /* Longitude hemisphere */
  if((token = strsep(fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%c"),
    &lon_hemisphere
  );

  /* Combine latitude degrees and minutes and adjust for hemisphere */
  position->latitude *= (lat_hemisphere == 'N' ? 1.0 : -1.0);
  position->longitude *= (lon_hemisphere == 'E' ? 1.0 : -1.0);

  if(position->latitude < -90.0 || position->latitude > 90.0)
    invalidity |= NMEA_INVALID_LATITUDE;

  if(position->longitude < -180.0 || position->longitude > 180.0)
    invalidity |= NMEA_INVALID_LONGITUDE;

  return invalidity;
}

unsigned int parse_checksum(char **fragment, char *checksum)
{
  unsigned int invalidity = 0;
  char *token;
  char checksum_buffer[3];

  /* Checksum, one byte as a hexadecimal string */
  if((token = strsep(fragment, "\n")) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%2s"),
    checksum_buffer
  );

  /* Convert the checksum from a hexadecimal string */
  *checksum = strtoul(checksum_buffer, NULL, 16);

  return invalidity;
}

// $GPRMC,070812.000,A,3923.1196,N,11937.6931,W,0.09,283.05,231115,,,A*74
unsigned int nmea_parse_gprmc(char *sentence, nmea_gprmc_t *data)
{
  unsigned int invalidity = 0;
  char checksum;
  char *fragment = sentence;
  char *token;

  /* Checksum before modifying the string with strsep */
  checksum = nmea_checksum(sentence);

  /* Pre-amble of "$GPRMC" */
  if(strncmp_P(fragment, PSTR("$GPRMC,"), 7) != 0)
    return NMEA_INVALID_TYPE;

  /* Skip over the pre-amble */
  fragment += 7;

  /* Zero-fill the return data structure */
  memset(data, 0, sizeof(*data));

  /* Current UTC time */
  if((invalidity |= parse_time(&fragment, &data->time)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Status of fix: A = Valid; V = Invalid */
  if((invalidity |= parse_char(&fragment, &data->status)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  if(data->status != 'A')
    invalidity |= NMEA_INVALID_STATUS;

  /* Latitude and longitude */
  if((invalidity |= parse_position(&fragment, &data->position)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Speed in knots */
  if((invalidity |= parse_float(&fragment, &data->velocity.speed)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Heading in degrees */
  if((invalidity |= parse_float(&fragment, &data->velocity.heading)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Current UTC date */
  if((invalidity |= parse_date(&fragment, &data->date)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Magnetic variation in degrees (unused) */
  if((token = strsep(&fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  /* Magnetic variation compass direction (unused) */
  if((token = strsep(&fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  /* Mode: A = Autonomous operation; N = Data not valid */
  if((invalidity |= parse_char(&fragment, &data->mode)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  if(data->mode != 'A')
    invalidity |= NMEA_INVALID_STATUS;

    /* Checksum */
    if((invalidity |= parse_checksum(&fragment, &data->checksum)) == NMEA_INVALID_SENTENCE)
      return NMEA_INVALID_SENTENCE;

  /* Verify that the checksum of the string matches the stored one */
  if(data->checksum != checksum)
    invalidity |= NMEA_INVALID_CHECKSUM;

  return invalidity;
}

// $GPGGA,070812.000,3923.1196,N,11937.6931,W,1,10,0.81,1773.2,M,-21.2,M,,*62
unsigned int nmea_parse_gpgga(char *sentence, nmea_gpgga_t *data)
{
  char checksum;
  unsigned int invalidity = 0;
  char *fragment = sentence;
  char *token;
  char unit;

  /* Checksum before modifying the string with strsep */
  checksum = nmea_checksum(sentence);

  /* Pre-amble of "$GPGGA" */
  if(strncmp_P(fragment, PSTR("$GPGGA,"), 7) != 0)
    return NMEA_INVALID_TYPE;

  /* Skip over the pre-amble */
  fragment += 7;

  /* Zero-fill the return data structure */
  memset(data, 0, sizeof(*data));

  /* Current UTC time */
  if((invalidity |= parse_time(&fragment, &data->time)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Latitude and longitude */
  if((invalidity |= parse_position(&fragment, &data->position)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  if((invalidity |= parse_uint8(&fragment, &data->fix_quality)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  if((invalidity |= parse_uint8(&fragment, &data->satellites_tracked)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  if((invalidity |= parse_float(&fragment, &data->hdop)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  if((invalidity |= parse_float(&fragment, &data->altitude)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Unit for altitude; should always be 'M' */
  if((invalidity |= parse_char(&fragment, &unit)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  if(unit != 'M')
    invalidity |= NMEA_INVALID_UNIT;

  if((invalidity |= parse_float(&fragment, &data->geoid_height)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Unit for geoid_height; should always be 'M' */
  if((invalidity |= parse_char(&fragment, &unit)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  if(unit != 'M')
    invalidity |= NMEA_INVALID_UNIT;

  /* unused */
  if((token = strsep(&fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  /* unused */
  if((token = strsep(&fragment, delim)) == NULL)
    return NMEA_INVALID_SENTENCE;

  /* Checksum */
  if((invalidity |= parse_checksum(&fragment, &data->checksum)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Verify that the checksum of the string matches the stored one */
  if(data->checksum != checksum)
    invalidity |= NMEA_INVALID_CHECKSUM;

  return invalidity;
}

// $GPGSA,A,3,28,09,08,13,19,30,07,27,11,05,,,1.13,0.81,0.79*03
unsigned int nmea_parse_gpgsa(char *sentence, nmea_gpgsa_t *data)
{
  char checksum;
  unsigned int invalidity = 0;
  char *fragment = sentence;
  int index;

  /* Checksum before modifying the string with strsep */
  checksum = nmea_checksum(sentence);

  /* Pre-amble of "$GPGSA" */
  if(strncmp_P(fragment, PSTR("$GPGSA,"), 7) != 0)
    return NMEA_INVALID_TYPE;

  /* Skip over the pre-amble */
  fragment += 7;

  /* Zero-fill the return data structure */
  memset(data, 0, sizeof(*data));

  /* Mode */
  if((invalidity |= parse_char(&fragment, &data->mode)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Fix type */
  if((invalidity |= parse_char(&fragment, &data->fix_type)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  for(index=0; index<12; index++)
  {
    if((invalidity |= parse_uint8(&fragment, &data->satellite_prn[index])) == NMEA_INVALID_SENTENCE)
      return NMEA_INVALID_SENTENCE;
  }

  if((invalidity |= parse_float(&fragment, &data->pdop)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  if((invalidity |= parse_float(&fragment, &data->hdop)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  if((invalidity |= parse_float(&fragment, &data->vdop)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Checksum */
  if((invalidity |= parse_checksum(&fragment, &data->checksum)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Verify that the checksum of the string matches the stored one */
  if(data->checksum != checksum)
    invalidity |= NMEA_INVALID_CHECKSUM;

  return invalidity;
}

// $GPGSV,4,1,13,07,66,049,21,30,62,322,20,28,48,239,23,09,41,161,22*74
unsigned int nmea_parse_gpgsv(char *sentence, nmea_gpgsv_t *data)
{
  char checksum;
  unsigned int invalidity = 0;
  char *fragment = sentence;
  int index;

  /* Checksum before modifying the string with strsep */
  checksum = nmea_checksum(sentence);

  /* Pre-amble of "$GPGSV" */
  if(strncmp_P(fragment, PSTR("$GPGSV,"), 7) != 0)
    return NMEA_INVALID_TYPE;

  /* Skip over the pre-amble */
  fragment += 7;

  /* Zero-fill the return data structure */
  memset(data, 0, sizeof(*data));

  /* Sentence total */
  if((invalidity |= parse_uint8(&fragment, &data->sentence_total)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Sentence number */
  if((invalidity |= parse_uint8(&fragment, &data->sentence_number)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Satellites in view */
  if((invalidity |= parse_uint8(&fragment, &data->satellites_in_view)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Array of information for 4 satellites in view */
  int start_index = 4 * (data->sentence_number-1);
  int end_index = 4 * data->sentence_number;
  if(end_index >= data->satellites_in_view)
    end_index = data->satellites_in_view;
  for(index=start_index; index < end_index; index++)
  {
    nmea_gpgsv_satellite_t *satellite = &data->satellite[index % 4];
    satellite->index = index;
    if((invalidity |= parse_uint8(&fragment, &satellite->prn)) == NMEA_INVALID_SENTENCE)
      return NMEA_INVALID_SENTENCE;
    if((invalidity |= parse_uint8(&fragment, &satellite->altitude)) == NMEA_INVALID_SENTENCE)
      return NMEA_INVALID_SENTENCE;
    if((invalidity |= parse_uint16(&fragment, &satellite->azimuth)) == NMEA_INVALID_SENTENCE)
      return NMEA_INVALID_SENTENCE;
    if((invalidity |= parse_uint8(&fragment, &satellite->snr)) == NMEA_INVALID_SENTENCE)
      return NMEA_INVALID_SENTENCE;
  }

  /* Checksum */
  if((invalidity |= parse_checksum(&fragment, &data->checksum)) == NMEA_INVALID_SENTENCE)
    return NMEA_INVALID_SENTENCE;

  /* Verify that the checksum of the string matches the stored one */
  if(data->checksum != checksum)
    invalidity |= NMEA_INVALID_CHECKSUM;

  return invalidity;
}
