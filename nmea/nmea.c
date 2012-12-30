/*
 * nmea.c
 *
 *  Created on: Dec 25, 2012
 *      Author: jcole
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "nmea.h"

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

unsigned int nmea_parse_gprmc(char *sentence, nmea_gprmc_t *data)
{
  char sentence_checksum;
  float lat_degrees;
  float lat_minutes;
  char  lat_hemisphere;
  float lon_degrees;
  float lon_minutes;
  char  lon_hemisphere;
  char  *token;
  static char *comma = ",";
  char checksum_buffer[3];
  unsigned int invalidity = 0;

  /* Pre-amble of "$GPRMC" */
  if(strncmp_P(sentence, PSTR("$GPRMC,"), 7) != 0)
    return NMEA_INVALID_TYPE;

  sentence_checksum = nmea_checksum(sentence);

  /* Skip over the pre-amble */
  sentence += 7;

  /* Zero-fill the return data structure */
  memset(data, 0, sizeof(nmea_gprmc_t));

  /* Current UTC time */
  if((token = strsep(&sentence, comma)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%02hhd%02hhd%02hhd.%03hd"),
    &data->hour,
    &data->minute,
    &data->second,
    &data->millisecond
  );

  /* Status of fix: A = Valid; V = Invalid */
  if((token = strsep(&sentence, comma)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%c"),
    &data->status
  );

  if(data->status != 'A')
    invalidity |= NMEA_INVALID_STATUS;

  /* Latitude degrees and minutes */
  if((token = strsep(&sentence, comma)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%02f%07f"),
    &lat_degrees,
    &lat_minutes
  );

  /* Latitude hemisphere */
  if((token = strsep(&sentence, comma)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%c"),
    &lat_hemisphere
  );

  /* Combine latitude degrees and minutes and adjust for hemisphere */
  data->latitude = (lat_degrees + (lat_minutes / 60.0))
    * (lat_hemisphere == 'N' ? 1.0 : -1.0);

  if(data->latitude < -90.0 || data->latitude > 90.0)
    invalidity |= NMEA_INVALID_LATITUDE;

  /* Longitude degrees and minutes */
  if((token = strsep(&sentence, comma)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%03f%07f"),
    &lon_degrees,
    &lon_minutes
  );

  /* Longitude hemisphere */
  if((token = strsep(&sentence, comma)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%c"),
    &lon_hemisphere
  );

  /* Combine longitude degrees and minutes and adjust for hemisphere */
  data->longitude = (lon_degrees + (lon_minutes / 60.0))
    * (lon_hemisphere == 'E' ? 1.0 : -1.0);

  if(data->longitude < -180.0 || data->longitude > 180.0)
    invalidity |= NMEA_INVALID_LONGITUDE;

  /* Speed in knots */
  if((token = strsep(&sentence, comma)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%f"),
    &data->speed
  );

  /* Heading in degrees */
  if((token = strsep(&sentence, comma)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%f"),
    &data->heading
  );

  /* Current UTC date */
  if((token = strsep(&sentence, comma)) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%02hhd%02hhd%02hd"),
    &data->date,
    &data->month,
    &data->year
  );

  /* Since 1980 is long past, if we see this year it's a good assumption
   * that the data is invalid.
   */
  if(data->year == 80)
    invalidity |= NMEA_INVALID_DATE;

  /* Adjust year based on GPS epoch of 1980 */
  data->year += (data->year) < 80 ? 2000 : 1900;

  /* Magnetic variation in degrees (unused) */
  if((token = strsep(&sentence, comma)) == NULL)
    return NMEA_INVALID_SENTENCE;

  /* Magnetic variation compass direction */
  if((token = strsep(&sentence, comma)) == NULL)
    return NMEA_INVALID_SENTENCE;

  /* Mode: A = Autonomous operation; N = Data not valid */
  if((token = strsep(&sentence, "*")) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%c"),
    &data->mode
  );

  if(data->mode != 'A')
    invalidity |= NMEA_INVALID_STATUS;

  /* Checksum, one byte as a hexadecimal string */
  if((token = strsep(&sentence, "\n")) == NULL)
    return NMEA_INVALID_SENTENCE;

  sscanf_P(token,
    PSTR("%2s"),
    checksum_buffer
  );

  /* Convert the checksum from a hexadecimal string */
  data->checksum = strtoul(checksum_buffer, NULL, 16);

  /* Verify that the checksum of the string matches the stored one */
  if(data->checksum != sentence_checksum)
    invalidity |= NMEA_INVALID_CHECKSUM;

  return(invalidity);
}

