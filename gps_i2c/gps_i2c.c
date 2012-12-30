/*
 * i2c_gps.c
 *
 *  Created on: Dec 27, 2012
 *      Author: jcole
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

uart_t *u0;
uart_t *u1;
rtc_datetime_24h_t current_dt;
uint8_t *current_dt_p;
uint8_t gps_uart_data_ready = 0;
uint8_t nmea_sentence_ready = 0;
char nmea_sentence[128];
char *nmea_sentence_p = nmea_sentence;

#define I2C_WAIT_CLEAR(v, b)  while(!((v) & _BV((b))))
#define I2C_WAIT_SET(v, b)    while((v) & _BV((b)))

uint8_t handle_i2c_slave_tx(uint8_t status, i2c_mode_t last_mode, i2c_mode_t current_mode)
{
  if(status == TW_ST_SLA_ACK)
  {
    current_dt_p = (uint8_t *)&current_dt;
  }

  if(status == TW_ST_SLA_ACK || status == TW_ST_DATA_ACK || status == TW_ST_DATA_NACK)
    TWDR = *current_dt_p++;

  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);

  return 0;
}

uint8_t handle_i2c_slave_rx(uint8_t status, i2c_mode_t last_mode, i2c_mode_t current_mode)
{
  uint8_t i2c_register;

  if(last_mode != current_mode)
  {
    current_dt_p = (uint8_t *)&current_dt;
  }

  if(status == TW_SR_DATA_ACK || status == TW_SR_DATA_NACK)
    i2c_register = TWDR;

  return 0;
}

#define NMEA_DISCARD (NMEA_INVALID_DATE | NMEA_INVALID_TIME)
void handle_nmea_sentence(void)
{
  nmea_gprmc_t gprmc;
  unsigned int invalid;

  if(strncmp_P(nmea_sentence, PSTR("$GPRMC,"), 7) != 0)
    return;

  printf("NMEA: %s\n", nmea_sentence);
  if((invalid = nmea_parse_gprmc(nmea_sentence, &gprmc)) & NMEA_DISCARD)
  {
    printf("Invalid NMEA: %04x\n", invalid);
    //return;
  }

  printf("NMEA Time: %04d-%02d-%02d %02d:%02d:%02d\n",
    gprmc.year,
    gprmc.month,
    gprmc.date,
    gprmc.hour,
    gprmc.minute,
    gprmc.second
  );

  current_dt.year   = gprmc.year;
  current_dt.month  = gprmc.month;
  current_dt.date   = gprmc.date;
  current_dt.hour   = gprmc.hour;
  current_dt.minute = gprmc.minute;
  current_dt.second = gprmc.second;
  current_dt.day_of_week =
      rtc_find_dow(gprmc.year, gprmc.month, gprmc.date);

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
      uart_puts(u0, "Resetting nmea_sentence due to overflow\n");
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
    if(data & UART_BUFFER_OVERFLOW) uart_puts(u0, "UART_BUFFER_OVERFLOW\n");
    if(data & UART_OVERRUN_ERROR) uart_puts(u0, "UART_OVERRUN_ERROR\n");
    if(data & UART_FRAME_ERROR) uart_puts(u0, "UART_FRAME_ERROR\n");
  }
}

int main(void)
{
  _delay_ms(1000);

  u0 = uart_init("0", UART_BAUD_SELECT(38400, F_CPU));
  uart_init_stdout(u0);

  u1 = uart_init("1", UART_BAUD_SELECT(9600, F_CPU));
  uart_set_rx_callback(u1, handle_gps_uart_input);

  i2c_init();
  i2c_slave_init(0x60, I2C_ADDRESS_MASK_SINGLE, I2C_GCALL_DISABLED);
  i2c_global.sr_callback = handle_i2c_slave_rx;
  i2c_global.st_callback = handle_i2c_slave_tx;

  sei();

  printf("\n\nBooted up!\n");

  uart_puts(u1, "\r\n");

  while(1)
  {
    //uart_puts(u0, "Test on UART 0!\r\n");
    //printf("Bytes available on UART 0: %d\n", uart_data_ready(u0));
    //printf("Bytes available on UART 1: %d\n", uart_data_ready(u1));
    //uart_puts(u1, "Test on UART 1!\r\n");
    /*
    if(uart_data_ready(u1))
    {
      while((c = uart_getc(u1)) != UART_NO_DATA)
        uart_putc(u0, c);
    }
    */
    handle_gps_uart_parsing(u1);

    //_delay_ms(10);
  }

  return(0);
}
