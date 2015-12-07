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
#include <inttypes.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <uart.h>
#include <i2c.h>
#include <rtc.h>
#include <nmea.h>

//#define NMEA_DEBUG_SENTENCES
//#define NMEA_DEBUG_RMC
//#define NMEA_DEBUG_GGA
//#define NMEA_DEBUG_GSA
//#define NMEA_DEBUG_GSV

uart_t *u0;
uart_t *u1;
uint16_t current_time_ms = 0, last_time_ms = 0;
struct {
  rtc_datetime_24h_t current_dt;
  uint8_t gps_signal_strength;
} data;
uint8_t *data_p;
uint8_t gps_uart_data_ready = 0;
uint8_t nmea_sentence_ready = 0;
char nmea_sentence[128];
char *nmea_sentence_p = nmea_sentence;

typedef struct _gps_state_t
{
  nmea_gprmc_t gprmc;
  nmea_gpgga_t gpgga;
  nmea_gpgsa_t gpgsa;
  nmea_gpgsv_t gpgsv[4];
} gps_state_t;

gps_state_t gps_state;

#define I2C_WAIT_CLEAR(v, b)  while(!((v) & _BV((b))))
#define I2C_WAIT_SET(v, b)    while((v) & _BV((b)))

uint8_t handle_i2c_slave_tx(uint8_t status, i2c_mode_t last_mode, i2c_mode_t current_mode)
{
  if(status == TW_ST_SLA_ACK)
  {
    data.current_dt.year   = gps_state.gprmc.date.year;
    data.current_dt.month  = gps_state.gprmc.date.month;
    data.current_dt.date   = gps_state.gprmc.date.day;
    data.current_dt.hour   = gps_state.gprmc.time.hour;
    data.current_dt.minute = gps_state.gprmc.time.minute;
    data.current_dt.second = gps_state.gprmc.time.second;
    data.current_dt.millisecond =
        current_time_ms < 1000 ? current_time_ms : 0;
    data.current_dt.day_of_week = rtc_find_dow(
        gps_state.gprmc.date.year,
        gps_state.gprmc.date.month,
        gps_state.gprmc.date.day);
    data.gps_signal_strength = gps_state.gpgga.satellites_tracked <= 9 ?
        gps_state.gpgga.satellites_tracked : 9;
    data_p = (uint8_t *)&data;
  }

  if(status == TW_ST_SLA_ACK || status == TW_ST_DATA_ACK || status == TW_ST_DATA_NACK)
    TWDR = *data_p++;

  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);

  return 0;
}

uint8_t handle_i2c_slave_rx(uint8_t status, i2c_mode_t last_mode, i2c_mode_t current_mode)
{
  uint8_t i2c_register;

  if(last_mode != current_mode)
  {
    data_p = (uint8_t *)&data;
  }

  if(status == TW_SR_DATA_ACK || status == TW_SR_DATA_NACK)
    i2c_register = TWDR;

  return 0;
}

void print_date(nmea_date_t *date)
{
  printf("%04d-%02d-%02d",
    date->year,
    date->month,
    date->day
  );
}

void print_time(nmea_time_t *time)
{
  printf("%02d:%02d:%02d.%03d",
    time->hour,
    time->minute,
    time->second,
    time->millisecond
  );
}

void print_nmea_gprmc(nmea_gprmc_t *gprmc)
{
  printf("NMEA RMC:\n");
  printf("  Status: %c\n", gprmc->status);
  printf("  Mode: %c\n", gprmc->mode);
  printf("  Time: ");
  print_date(&gprmc->date);
  printf(" ");
  print_time(&gprmc->time);
  printf("\n");
  printf("  Position: %f, %f\n",
      gprmc->position.latitude,
      gprmc->position.longitude);
  printf("  Velocity: %f, %f\n",
      gprmc->velocity.speed,
      gprmc->velocity.heading);
  printf("  Checksum: %02x\n", gprmc->checksum);
  printf("\n");
}

// $GPRMC,070812.000,A,3923.1196,N,11937.6931,W,0.09,283.05,231115,,,A*74
void handle_nmea_gprmc(char *sentence)
{
  unsigned int invalidity;
  nmea_gprmc_t gprmc;

  invalidity = nmea_parse_gprmc(sentence, &gprmc);
  if(invalidity != 0)
  {
    printf("NMEA RMC Invalid: %04x; Checksum: %02x\n", invalidity, gprmc.checksum);
  }

  gps_state.gprmc = gprmc;

#ifdef NMEA_DEBUG_RMC
  print_nmea_gprmc(&gprmc);
#endif // NMEA_DEBUG_RMC

  if(invalidity & (NMEA_INVALID_DATE | NMEA_INVALID_TIME))
    return;
}

void print_nmea_gpgga(nmea_gpgga_t *gpgga)
{
  printf("NMEA GGA:\n");
  printf("  Time: ");
  print_time(&gpgga->time);
  printf("\n");
  printf("  Position: %f, %f\n",
      gpgga->position.latitude,
      gpgga->position.longitude);
  printf("  Fix Quality: %d\n", gpgga->fix_quality);
  printf("  Satellites Tracked: %d\n", gpgga->satellites_tracked);
  printf("  HDOP: %f\n", gpgga->hdop);
  printf("  Altitude: %f\n", gpgga->altitude);
  printf("  Geoid Distance: %f\n", gpgga->geoid_height);
  printf("  Checksum: %02x\n", gpgga->checksum);
  printf("\n");
}

// $GPGGA,070812.000,3923.1196,N,11937.6931,W,1,10,0.81,1773.2,M,-21.2,M,,*62
void handle_nmea_gpgga(char *sentence)
{
  unsigned int invalidity;
  nmea_gpgga_t gpgga;

  invalidity = nmea_parse_gpgga(sentence, &gpgga);
  if(invalidity != 0)
  {
    printf("NMEA GGA Invalid: %04x; Checksum: %02x\n", invalidity, gpgga.checksum);
  }

  gps_state.gpgga = gpgga;

#ifdef NMEA_DEBUG_GGA
  print_nmea_gpgga(&gpgga);
#endif // NMEA_DEBUG_GGA
}

void print_nmea_gpgsa(nmea_gpgsa_t *gpgsa)
{
  printf("NMEA GSA:\n");
  printf("  Mode: %c\n", gpgsa->mode);
  printf("  Fix Type: %c\n", gpgsa->fix_type);
  printf("  Satellite PRNs:");
  for(int index=0; index<12; index++)
    printf(" %d", gpgsa->satellite_prn[index]);
  printf("\n");
  printf("  PDOP: %f\n", gpgsa->pdop);
  printf("  HDOP: %f\n", gpgsa->hdop);
  printf("  VDOP: %f\n", gpgsa->vdop);
  printf("  Checksum: %02x\n", gpgsa->checksum);
  printf("\n");
}

// $GPGSA,A,3,28,09,08,13,19,30,07,27,11,05,,,1.13,0.81,0.79*03
void handle_nmea_gpgsa(char *sentence)
{
  unsigned int invalidity;
  nmea_gpgsa_t gpgsa;

  invalidity = nmea_parse_gpgsa(sentence, &gpgsa);
  if(invalidity != 0)
  {
    printf("NMEA GSA Invalid: %04x; Checksum: %02x\n", invalidity, gpgsa.checksum);
  }

  gps_state.gpgsa = gpgsa;

#ifdef NMEA_DEBUG_GSA
  print_nmea_gpgsa(&gpgsa);
#endif // NMEA_DEBUG_GSA
}

void print_nmea_gpgsv(nmea_gpgsv_t *gpgsv)
{
  printf("NMEA GSV:\n");
  printf("  Sentence: %d of %d\n", gpgsv->sentence_number, gpgsv->sentence_total);
  printf("  Satellites in view (of %d):\n", gpgsv->satellites_in_view);
  for(int i=0; i<4; i++)
  {
    if(gpgsv->satellite[i].prn)
    {
      printf("    %2d: PRN: %2d, Alt: %2d, Az: %3d, SNR: %2d\n",
        gpgsv->satellite[i].index,
        gpgsv->satellite[i].prn,
        gpgsv->satellite[i].altitude,
        gpgsv->satellite[i].azimuth,
        gpgsv->satellite[i].snr);
    }
  }
  printf("  Checksum: %02x\n", gpgsv->checksum);
  printf("\n");
}

void print_nmea_gpgsv_summary(nmea_gpgsv_t *gpgsv)
{
  printf("Satellites in view (%d):\n", gpgsv[0].satellites_in_view);
  for(int s=0; s<gpgsv[0].sentence_total; s++)
  {
    if(gpgsv[s].satellites_in_view > 0)
    {
      for(int i=0; i<4; i++)
      {
        nmea_gpgsv_satellite_t *satellite = &gpgsv[s].satellite[i];
        if(satellite->prn)
        {
          printf("  %2d: PRN: %2d, Alt: %2d, Az: %3d, SNR: %2d\n",
            satellite->index,
            satellite->prn,
            satellite->altitude,
            satellite->azimuth,
            satellite->snr);
        }
      }
    }
  }
  printf("\n");
}

// $GPGSV,4,1,13,07,66,049,21,30,62,322,20,28,48,239,23,09,41,161,22*74
void handle_nmea_gpgsv(char *sentence)
{
  unsigned int invalidity;
  nmea_gpgsv_t gpgsv;

  invalidity = nmea_parse_gpgsv(sentence, &gpgsv);
  if(invalidity != 0)
  {
    printf("NMEA GSV Invalid: %04x; Checksum: %02x\n", invalidity, gpgsv.checksum);
  }

  gps_state.gpgsv[gpgsv.sentence_number - 1] = gpgsv;

#ifdef NMEA_DEBUG_GSV
  print_nmea_gpgsv(&gpgsv);
#endif // NMEA_DEBUG_GSV
}

void handle_nmea_sentence(void)
{
#ifdef NMEA_DEBUG_SENTENCES
  printf("NMEA: %s", nmea_sentence);
#endif // NMEA_DEBUG_SENTENCES

  if(strncmp_P(nmea_sentence, PSTR("$GPRMC,"), 7) == 0)
  {
    handle_nmea_gprmc(nmea_sentence);
    printf("Time at reset was ms = %d\n", last_time_ms);
    //current_time_ms = 0;
  }
  else if(strncmp_P(nmea_sentence, PSTR("$GPGGA,"), 7) == 0)
  {
    handle_nmea_gpgga(nmea_sentence);
  }
  else if(strncmp_P(nmea_sentence, PSTR("$GPGSA,"), 7) == 0)
  {
    handle_nmea_gpgsa(nmea_sentence);
  }
  else if(strncmp_P(nmea_sentence, PSTR("$GPGSV,"), 7) == 0)
  {
    handle_nmea_gpgsv(nmea_sentence);
  }
}

void handle_gps_uart_input(uart_t *uart)
{
  gps_uart_data_ready = 1;
}

void handle_gps_uart_parsing(uart_t *uart)
{
  unsigned int data;
  if(uart_data_ready(uart))
  {
    if(nmea_sentence_p >= nmea_sentence+127)
    {
      uart_puts(u0, "Resetting nmea_sentence due to overflow\r\n");
      *nmea_sentence = 0;
      nmea_sentence_p = nmea_sentence;
    }
    while(((data = uart_getc(uart)) & 0xff00) == 0)
    {
      if(data == '\r') continue;
      *nmea_sentence_p++ = data;

      if(data == '\n')
      {
        *nmea_sentence_p++ = 0;
        handle_nmea_sentence();
        *nmea_sentence = 0;
        nmea_sentence_p = nmea_sentence;
      }
    }
    //if(data & UART_BUFFER_OVERFLOW) uart_puts(u0, "UART_BUFFER_OVERFLOW\r\n");
    //if(data & UART_OVERRUN_ERROR) uart_puts(u0, "UART_OVERRUN_ERROR\r\n");
    //if(data & UART_FRAME_ERROR) uart_puts(u0, "UART_FRAME_ERROR\r\n");
  }
}

void print_gps_state(void)
{
  print_nmea_gprmc(&gps_state.gprmc);
  print_nmea_gpgga(&gps_state.gpgga);
  print_nmea_gpgsa(&gps_state.gpgsa);
  print_nmea_gpgsv_summary(gps_state.gpgsv);
}

ISR(TIMER0_COMPA_vect)
{
  TCNT0 = 0;
  current_time_ms += 1;
}

ISR(PCINT3_vect)
{
  if(PIND & _BV(PD4))
  {
    last_time_ms = current_time_ms;
    current_time_ms = 0;
  }
}

void init_timer_hz(uint16_t hz)
{
  /* Set pre-scaler to /64 */
  TCCR0B |= _BV(CS01) | _BV(CS00);

  /* Force Output Compare A */
  TCCR0B |= _BV(FOC0A);

  /* Output Compare Interrupt Enable A */
  TIMSK0 |= _BV(OCIE0A);

  /* Set initial counter value */
  TCNT0 = 0;

  /* Set Output Compare Register to aim for hz interrupt */
  OCR0A = (F_CPU / 64) / hz ;
}

int main(void)
{
  char gps_state_printed = 0;
  _delay_ms(1000);

  memset(&gps_state, 0, sizeof(gps_state));

  u0 = uart_init("0", UART_BAUD_SELECT(38400, F_CPU));
  uart_init_stdout(u0);

  u1 = uart_init("1", UART_BAUD_SELECT(9600, F_CPU));
  uart_set_rx_callback(u1, handle_gps_uart_input);

  i2c_init();
  i2c_slave_init(0x60, I2C_ADDRESS_MASK_SINGLE, I2C_GCALL_DISABLED);
  i2c_global.sr_callback = handle_i2c_slave_rx;
  i2c_global.st_callback = handle_i2c_slave_tx;

  current_time_ms = 0;
  init_timer_hz(1000 + 24);

  PIND |= _BV(PD4);
  PORTD &= ~_BV(PD4);
  PCMSK3 |= _BV(PCINT28);
  PCICR |= _BV(PCIE3);

  sei();

  printf("\n\nBooted up!\n");

  uart_puts(u1, "\r\n");

  while(1)
  {
    handle_gps_uart_parsing(u1);

    if(gps_state.gprmc.time.second % 10 == 0)
    {
      if(gps_state_printed == 0)
      {
        print_gps_state();
        gps_state_printed = 1;
      }
    }
    else
    {
      gps_state_printed = 0;
    }
  }

  return 0;
}
