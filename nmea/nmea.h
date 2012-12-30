/*
 * nmea.h
 *
 *  Created on: Dec 25, 2012
 *      Author: jcole
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
#define NMEA_INVALID_STATUS         0x4000
#define NMEA_INVALID_CHECKSUM       0x8000

typedef struct _nmea_gprmc_t
{
  char status;
  char mode;
  uint16_t year;
  uint8_t month;
  uint8_t date;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint16_t millisecond;
  double latitude;
  double longitude;
  float speed;
  float heading;
  char checksum;
} nmea_gprmc_t;

extern unsigned int nmea_parse_gprmc(char *sentence, nmea_gprmc_t *data);
#endif /* NMEA_H_ */
