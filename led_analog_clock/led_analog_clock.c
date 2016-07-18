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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#define I2C_SCL_CLOCK 400000L
#include <i2c.h>

#include <rtc.h>
#include <rtc_ds1307.h>
#include <uart.h>
#include <led_sequencer.h>
#include <led_charlieplex.h>

#include "led_analog_clock_v2.h"
#define LED_DISPLAY led_analog_clock_v2
#include "led_mapping.h"

const int gps_leap_second_offset = 1;

#define COMMAND_BUFFER_SIZE 32

typedef struct _configuration_t
{
  int8_t tz_offset;
  int8_t dst_offset;
} configuration_t;

configuration_t configuration;

led_sequence_step_t *step_hour   = NULL;
led_sequence_step_t *step_minute = NULL;
led_sequence_step_t *step_second = NULL;

rtc_device_t *rtc = &rtc_ds1307;
rtc_datetime_24h_t current_time;

rtc_dst_date_t usa_dst_dates[] =
{
  {2012,  3, 11, 11, 4},
  {2013,  3, 10, 11, 3},
  {2014,  3,  9, 11, 2},
  {2015,  3,  8, 11, 1},
  {2016,  3, 13, 11, 6},
  {2017,  3, 12, 11, 5},
  {2018,  3, 11, 11, 4},
  {2019,  3, 10, 11, 3},
  {0, 0, 0, 0, 0}
};

uint8_t last_hour   = 0;
uint8_t last_minute = 0;
uint8_t last_second = 0;

struct {
  rtc_datetime_24h_t dt;
  uint8_t gps_signal_strength;
} gps_data;

uint16_t time_elapsed_since_gps_sync = 0;

volatile uint8_t ready_flags = 0;

#define READY_UART_DATA  1
#define READY_UPDATE_HMS 2

/**
 * Pin change interrupt attached to SQW (square wave) output pin from DS1307.
 */
ISR(PCINT2_vect)
{
  /*
   * This interrupt is called on both rising and falling edges of SQW, but
   * we want to trigger only on falling edge as per DS1307 specifications,
   * which state that the clock time will be updated just before the SQW
   * is brought low.
   */
  if((PINC & _BV(PC6)) == 0)
  {
    ready_flags |= READY_UPDATE_HMS;
  }
}

/**
 * Callback function for UART input.  Don't do anything with the input just
 * yet, but mark that we've received something to be handled outside of the
 * interrupt handler.
 */
void notice_uart_input(uart_t *uart)
{
  ready_flags |= READY_UART_DATA;
}

/**
 * Write the current time (in rtc_datetime_24h_t format) to a remote LCD
 * device over I2C. This is essentially fire-and-forget, as there's no check
 * that the remote device exists or received the data correctly.
 */
void write_remote_lcd(rtc_datetime_24h_t *dt, uint8_t gps_signal_strength)
{
  if(0 == i2c_start(0x70, I2C_WRITE))
  {
    i2c_write_array((uint8_t *)dt, sizeof(*dt));
  }
  i2c_write(gps_signal_strength);
  i2c_stop();
}

uint8_t jit_qhour_loop_25ms(led_sequence_step_t *step, uint8_t status)
{
  static uint8_t i;
  if(status == LED_SEQUENCER_JIT_INITIAL) i = 0;

  if(i >= 48)
    return LED_SEQUENCER_JIT_DEQUEUE;

  led_sequencer_sequence_modify_step(step, led_mapping_qhour[i++ % 48], 25);

  return LED_SEQUENCER_JIT_CONTINUE;
}

uint8_t jit_minute_loop_10ms(led_sequence_step_t *step, uint8_t status)
{
  static uint8_t i;
  if(status == LED_SEQUENCER_JIT_INITIAL) i = 0;

  if(i >= 60)
    return LED_SEQUENCER_JIT_DEQUEUE;

  led_sequencer_sequence_modify_step(step, led_mapping_minute[i++ % 60], 10);

  return LED_SEQUENCER_JIT_CONTINUE;
}

uint8_t jit_minute_loop_20ms(led_sequence_step_t *step, uint8_t status)
{
  static uint8_t i;
  if(status == LED_SEQUENCER_JIT_INITIAL) i = 0;

  if(i >= 60)
    return LED_SEQUENCER_JIT_DEQUEUE;

  led_sequencer_sequence_modify_step(step, led_mapping_minute[i++ % 60], 20);

  return LED_SEQUENCER_JIT_CONTINUE;
}

uint8_t jit_minute_loop_reverse_20ms(led_sequence_step_t *step, uint8_t status)
{
  static uint8_t i;
  if(status == LED_SEQUENCER_JIT_INITIAL) i = 0;

  if(i >= 60)
    return LED_SEQUENCER_JIT_DEQUEUE;

  led_sequencer_sequence_modify_step(step, led_mapping_minute[(60 - i++) % 60], 20);

  return LED_SEQUENCER_JIT_CONTINUE;
}

uint8_t jit_inner_loop_10ms(led_sequence_step_t *step, uint8_t status)
{
  static uint8_t i;
  if(status == LED_SEQUENCER_JIT_INITIAL) i = 0;

  if(i >= 4)
    return LED_SEQUENCER_JIT_DEQUEUE;

  led_sequencer_sequence_modify_step(step, led_mapping_inner[i++ % 4], 10);

  return LED_SEQUENCER_JIT_CONTINUE;
}

uint8_t jit_around_all_bars_5ms_10x(led_sequence_step_t *step, uint8_t status)
{
  static uint8_t i;
  if(status == LED_SEQUENCER_JIT_INITIAL) i = 0;

  if(i >= 200)
    return LED_SEQUENCER_JIT_DEQUEUE;

  led_sequencer_sequence_modify_step(step, led_mapping_around_all_bars[i++ % 20], 5);

  return LED_SEQUENCER_JIT_CONTINUE;
}

uint8_t jit_random_all_bars_50ms_40x(led_sequence_step_t *step, uint8_t status)
{
  static uint8_t i;
  if(status == LED_SEQUENCER_JIT_INITIAL) i = 0;

  if(i++ >= 40)
    return LED_SEQUENCER_JIT_DEQUEUE;

  led_sequencer_sequence_modify_step(step, led_mapping_around_all_bars[rand() % 20], 50);

  return LED_SEQUENCER_JIT_CONTINUE;
}

uint8_t jit_random_all_bars_50ms_long(led_sequence_step_t *step, uint8_t status)
{
  static uint16_t i;
  if(status == LED_SEQUENCER_JIT_INITIAL) i = 0;

  if(i++ >= 10000)
    return LED_SEQUENCER_JIT_DEQUEUE;

  led_sequencer_sequence_modify_step(step, led_mapping_around_all_bars[rand() % 20], 50);

  return LED_SEQUENCER_JIT_CONTINUE;
}

void enqueue_secondly_show(void)
{
  /* No show */
}

void enqueue_minutely_show(void)
{
  led_sequencer_sequence_clear("M");
  led_sequencer_sequence_push_back_jit("M", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_10ms);
  led_sequencer_sequence_push_back_jit("M", LED_SEQUENCER_STEP_SHOW, "c", jit_inner_loop_10ms);
}

void enqueue_hourly_show(void)
{
  led_sequencer_sequence_clear("H");
  led_sequencer_sequence_push_back_jit("M", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_reverse_20ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_qhour_loop_25ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_random_all_bars_50ms_40x);
}

void enqueue_nye_show(void)
{
  led_sequencer_sequence_clear("H");
  led_sequencer_sequence_clear("M");
  led_sequencer_sequence_push_back_jit("M", LED_SEQUENCER_STEP_SHOW, "c", jit_random_all_bars_50ms_long);
  led_sequencer_sequence_push_back_jit("S", LED_SEQUENCER_STEP_SHOW, "c", jit_qhour_loop_25ms);
  led_sequencer_sequence_push_back_jit("S", LED_SEQUENCER_STEP_SHOW, "c", jit_qhour_loop_25ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_10ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_reverse_20ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_10ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_reverse_20ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_10ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_reverse_20ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_10ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_reverse_20ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_10ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_reverse_20ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_10ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_reverse_20ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_10ms);
  led_sequencer_sequence_push_back_jit("H", LED_SEQUENCER_STEP_SHOW, "c", jit_minute_loop_reverse_20ms);
}

/**
 * Save the time zone and daylight saving time configuration to EEPROM.
 */
void configuration_save()
{
  eeprom_write_block((void *)&configuration, (void *)0x00, sizeof(configuration_t));
}

/**
 * Restore a previously saved configuration, and if it doesn't exist configure
 * some sane defaults.
 */
void configuration_restore()
{
  eeprom_read_block((void *)&configuration, (void *)0x00, sizeof(configuration_t));

  if(configuration.tz_offset > 12 || configuration.tz_offset < -12)
    configuration.tz_offset = 0;

  if(configuration.dst_offset != 1 && configuration.dst_offset != 0)
    configuration.dst_offset = 0;
}

/**
 * Parse and then execute the "S" command from the user, which sets the time.
 */
void command_set(char *command)
{
  uint8_t rc;
  rtc_datetime_24h_t dt;

  rc = rtc_clock_stop(rtc);
  printf_P(PSTR("Halted clock, rc=%i\n"), rc);

  rtc_sqw_enable(rtc);
  rtc_sqw_rate(rtc, 1);

  rc = rtc_read(rtc, &dt);

  sscanf_P(command,
      PSTR("S %04d-%02hhd-%02hhd %02hhd:%02hhd:%02hhd\n"),
      &dt.year, &dt.month, &dt.date,
      &dt.hour, &dt.minute, &dt.second);

  dt.day_of_week = rtc_find_dow(dt.year, dt.month, dt.date);

  printf_P(PSTR("Setting time to %04i-%02i-%02i %02i:%02i:%02i (%s)\n"),
      dt.year, dt.month, dt.date,
      dt.hour, dt.minute, dt.second,
      rtc_dow_names[dt.day_of_week]);

  printf_P(PSTR("Trying to write RTC...\n"));
  rc = rtc_write(rtc, &dt);
  printf_P(PSTR("Wrote RTC, rc=%i\n"), rc);

  rc = rtc_clock_start(rtc);
  printf_P(PSTR("Started clock, rc=%i\n"), rc);
}

/**
 * Parse and then execute the "S" command from the user, which sets the time.
 */
void command_offset(char *command)
{
  int8_t tmp_tz_offset;
  int8_t tmp_dst_offset;

  sscanf_P(command, PSTR("O %hhd %hhd\n"), &tmp_tz_offset, &tmp_dst_offset);

  printf_P(PSTR("Setting offset to %i, dst to %i\n"),
    tmp_tz_offset, tmp_dst_offset);

  configuration.tz_offset = tmp_tz_offset;
  configuration.dst_offset   = tmp_dst_offset;

  configuration_save();
}

uint8_t gps_read_ram(uint8_t address, uint8_t length, unsigned char *data)
{
  uint8_t rc;
  uint8_t pos;

  /*
  rc = i2c_start(0x60, I2C_WRITE);
  if(rc) return 1;

  rc = i2c_write(address);
  if(rc) return 2;

  i2c_stop();
  */
  rc = i2c_rep_start(0x60, I2C_READ);
  if(rc) return rc;

  for(pos=0; pos < length; pos++, data++)
  {
    *data = (char)i2c_read(pos != (length-1));
  }
  i2c_stop();

  return 0;
}

uint8_t update_gps()
{
  uint8_t rc;
  rc = gps_read_ram(0, sizeof(gps_data), (unsigned char *)&gps_data);
  if(rc)
  {
    printf("Return from gps_read_ram: %d\n", rc);
  }
  return rc;
}

void command_get_gps()
{
  update_gps();

  printf_P(
    PSTR("GPS  : %04i-%02i-%02i %02i:%02i:%02i.%03i (%s) [offset %i]\n"),
    gps_data.dt.year, gps_data.dt.month,  gps_data.dt.date,
    gps_data.dt.hour, gps_data.dt.minute, gps_data.dt.second,
    gps_data.dt.millisecond,
    rtc_dow_names[gps_data.dt.day_of_week],
    gps_leap_second_offset);
}

/**
 * Execute the "G" command, which gets the current time and prints it.
 */
void command_get()
{
  rtc_datetime_24h_t offset_time;

  rtc_offset_time(&current_time, &offset_time,
    configuration.tz_offset + configuration.dst_offset);

  printf_P(
    PSTR("UTC  : %04i-%02i-%02i %02i:%02i:%02i (%s)\n"),
    current_time.year, current_time.month,  current_time.date,
    current_time.hour, current_time.minute, current_time.second,
    rtc_dow_names[current_time.day_of_week]);

  printf_P(
    PSTR("Local: %04i-%02i-%02i %02i:%02i:%02i (%s) [tz %i, dst %i]\n"),
    offset_time.year, offset_time.month,  offset_time.date,
    offset_time.hour, offset_time.minute, offset_time.second,
    rtc_dow_names[offset_time.day_of_week],
    configuration.tz_offset, configuration.dst_offset);

  command_get_gps();
}

void wait_ms(uint16_t milliseconds)
{
  while(milliseconds > 100) {
    _delay_ms(100);
    milliseconds -= 100;
  }

  while(milliseconds > 10)
  {
    _delay_ms(10);
    milliseconds -= 10;
  }

  while(milliseconds > 1)
  {
    _delay_ms(1);
    milliseconds -= 1;
  }
}

void command_set_from_gps(void)
{
  uint8_t rc;

  if(update_gps())
    return;

  if(gps_data.gps_signal_strength < 3)
  {
    printf_P(PSTR("GPS signal strength (%d) is unreliable. Aborting sync!\n"),
        gps_data.gps_signal_strength);
    return;
  }

  if(gps_data.dt.second < (59 - gps_leap_second_offset))
  {
    /* Adjust to the edge of the next second, don't deal with rollover */
    printf_P(PSTR("Sleeping for %d ms to synchronize...\n"),
        1000 - gps_data.dt.millisecond - 1);
    wait_ms(1000 - gps_data.dt.millisecond - 1);
    gps_data.dt.second += 1 + gps_leap_second_offset;
    gps_data.dt.millisecond = 0;
  }

  rc = rtc_clock_stop(rtc);
  printf_P(PSTR("Halted clock, rc=%i\n"), rc);

  rtc_sqw_enable(rtc);
  rtc_sqw_rate(rtc, 1);

  printf_P(PSTR("Setting time to %04i-%02i-%02i %02i:%02i:%02i (%s)\n"),
      gps_data.dt.year, gps_data.dt.month, gps_data.dt.date,
      gps_data.dt.hour, gps_data.dt.minute, gps_data.dt.second,
      rtc_dow_names[gps_data.dt.day_of_week]);

  printf_P(PSTR("Trying to write RTC...\n"));
  rc = rtc_write(rtc, &gps_data.dt);
  printf_P(PSTR("Wrote RTC, rc=%i\n"), rc);

  rc = rtc_clock_start(rtc);
  printf_P(PSTR("Started clock, rc=%i\n"), rc);
}

/**
 * Dump the first 16 bytes of the EEPROM in hex, for debugging.
 */
void command_eeprom_dump()
{
  uint16_t i;
  for(i=0; i<16; i++)
  {
    printf("%02x ", eeprom_read_byte((uint8_t *)i));
  }
  printf("\n");
}

/**
 * Dispatch a command to its handling function based on the single-letter
 * command.
 */
void handle_command(char *command)
{
  switch(command[0])
  {
  case 'S':
    command_set(command);
    break;
  case 's':
    command_set_from_gps();
    break;
  case 'G':
    command_get();
    break;
  case 'O':
    command_offset(command);
    break;
  case 'E':
    command_eeprom_dump();
    break;
  case 'D':
    led_sequencer_dump_sequence();
    break;
  case 'm':
    led_sequencer_halt();
    enqueue_minutely_show();
    led_sequencer_run();
    break;
  case 'h':
    led_sequencer_halt();
    enqueue_hourly_show();
    led_sequencer_run();
    break;
  case 'g':
    command_get_gps();
    break;
  default:
    break;
  }
}

/**
 * Handle input from the serial port, buffering a full line of input up to
 * COMMAND_BUFFER_SIZE in length.
 */
void handle_uart_input(uart_t *uart)
{
  static char command_buffer[COMMAND_BUFFER_SIZE] = "";
  char c, *s;

  c = uart_getc(uart);

  if(c & UART_NO_DATA)
    return;

  if(c == 0x03) /* Control-C */
  {
    uart_putc(uart, '\r');
    uart_putc(uart, '\n');
    command_buffer[0] = 0;
    return;
  }

  if(c == 0x08) /* Backspace */
  {
    /* Delete the last character from the buffer */
    command_buffer[strlen(command_buffer)-1] = 0;

    /* Remove the last character from the screen */
    uart_putc(uart, 0x08);
    uart_putc(uart, ' ');
    uart_putc(uart, 0x08);
    return;
  }

  /* Buffer full, discard more input */
  if(strlen(command_buffer) >= (COMMAND_BUFFER_SIZE-1))
    return;

  uart_putc(uart, c);

  if(c == '\r')
    uart_putc(uart, '\n');

  if(c != '\r' && c != '\n')
  {
    for(s=command_buffer; *s; s++);

    *s++ = c;
    *s++ = 0;
    return;
  }

  handle_command(command_buffer);
  command_buffer[0] = 0;
}

/**
 * Update the "h", "m", and "s" sequences, optionally queuing an animation
 * into "H" or "M" sequences, depending on the new time.  This function is
 * called once per second during the main loop after the interrupt triggered
 * by the time change marks update_hms_ready.
 *
 * The "h", "m", and "s" sequences are updated in-place in order to avoid
 * additional work and possible memory fragmentation from removing and
 * re-enqueuing the applicable LED.
 */
void update_hms(void)
{
  uint8_t rc;
  rtc_datetime_24h_t offset_time;

  if(++time_elapsed_since_gps_sync > 1777)
  {
    printf_P(PSTR("Maximum time limit exceeded since last GPS sync, syncing...\n"));
    command_set_from_gps();
    time_elapsed_since_gps_sync = 0;
  }

  rc = rtc_read(rtc, &current_time);
  if(rc) return;

  rtc_offset_time(&current_time, &offset_time,
    configuration.tz_offset + configuration.dst_offset);

  if(current_time.hour != last_hour)
  {
    configuration.dst_offset = rtc_find_dst_offset(offset_time, usa_dst_dates);
    rtc_offset_time(&current_time, &offset_time,
      configuration.tz_offset + configuration.dst_offset);
  }

  led_sequencer_halt();

  /*
   * If the second has changed since the last time the time was checked,
   * we should update the "second", "minute, and "hour" sequences.  In practice
   * this is nearly always the case, since the interrupts ensure we're only
   * really called when needed.
   */
  if(current_time.second != last_second)
  {
    led_sequencer_sequence_modify_step(step_second, led_mapping_minute[offset_time.second], 255);
    led_sequencer_sequence_modify_step(step_minute, led_mapping_minute[offset_time.minute], 255);
    led_sequencer_sequence_modify_step(step_hour,   led_mapping_qhour[((offset_time.hour % 12) * 4) + (offset_time.minute / 15)], 255);
    enqueue_secondly_show();
    last_second = current_time.second;
  }

  /* Do something nice for New Years */
  if(offset_time.month == 1 && offset_time.date == 1
		  && offset_time.hour == 0 && offset_time.minute == 0
		  && offset_time.second == 0)
  {
    enqueue_nye_show();
    last_hour   = current_time.hour;
    last_minute = current_time.minute;
  }

  /*
   * If the hour has changed, enqueue the hourly show.
   */
  else if(current_time.hour != last_hour)
  {
    enqueue_hourly_show();
    last_hour = current_time.hour;
  }

  /*
   * If the hour is still the same, enqueue the minutely show.
   */
  else if(current_time.minute != last_minute)
  {
    enqueue_minutely_show();
    last_minute = current_time.minute;
  }

  led_sequencer_run();
  update_gps();
  write_remote_lcd(&offset_time, gps_data.gps_signal_strength);
}

int main(void)
{
  uart_t *u0;

  /* Delay for one second to avoid multiple resets during programming. */
  _delay_ms(1000);

  /* Initialize USART0 and set up stdout to write to it. */
  u0 = uart_init("0", UART_BAUD_SELECT(38400, F_CPU));
  uart_init_stdout(u0);
  uart_set_rx_callback(u0, notice_uart_input);

  i2c_init();

  /*
   * Test if pullup is required on the I2C pins, and enable if the pins are
   * reading low. This allows external pullups to optionally be used, so that
   * for example the I2C bus can be pulled up to 3.3V to allow communication
   * between 3.3V and 5V devices at 3.3V.
   */
  if((PINC & (_BV(PC0) | _BV(PC1))) == 0)
  {
    DDRC  = _BV(PC0) | _BV(PC1);
    PORTC = _BV(PC0) | _BV(PC1);
  }

  sei();

  printf_P(PSTR("\n\nBooted!\n"));

  printf_P(PSTR("Reading saved configuration...\n\n"));
  configuration_restore();

  printf_P(PSTR(
    "Configuration:\n"
    "  tz_offset:  %2i\n"
    "  dst_offset: %2i\n"
    "\n"
  ), configuration.tz_offset, configuration.dst_offset);

  printf_P(PSTR(
    "Commands:\n"
    "  Get the current time:\n"
    "    G\n"
    "  Set the current time:\n"
    "    S YYYY-MM-DD hh:mm:ss\n"
    "  Set the timezone offset from UTC:\n"
    "    O <tz_offset> <dst_offset>\n"
    "  Set the time from GPS (if available):"
    "    s\n"
    "\n"
  ));

  rtc_init(rtc);
  rtc_sqw_enable(rtc);
  rtc_sqw_rate(rtc, 1);

  /* Enable pullup and interrupt for PC6/PCINT22, DS1307 SQW pin. */
  PORTC  |= _BV(PC6);
  PCMSK2 |= _BV(PCINT22);
  PCICR  |= _BV(PCIE2);

  /* Initialize the sequencer with 1000Hz tick rate. */
  led_sequencer_init(1000);

  /* Initialize and load the LED Analog Clock LED display. */
  led_charlieplex_init(&LED_DISPLAY);
  led_sequencer_push_front_matrix("c", &LED_DISPLAY);

  /*
   * Add empty sequences for hour, minute, second, hourly show, and minutely
   * show.
   */
  led_sequencer_push_back_sequence("h");
  led_sequencer_push_back_sequence("m");
  led_sequencer_push_back_sequence("s");
  led_sequencer_push_back_sequence("H");
  led_sequencer_push_back_sequence("M");

  /*
   * Initially read the time, set last_* for use later, and push a sequencer
   * step into the second, minute, and hour sequences.  The steps pushed
   * below are never removed, as they are modified in place in update_hms()
   * with each time change.
   */
  rtc_read(rtc, &current_time);
  last_hour   = current_time.hour;
  last_minute = current_time.minute;
  last_second = current_time.second;
  step_hour   = led_sequencer_sequence_push_back_step("h", LED_SEQUENCER_STEP_SHOW, "c", led_mapping_qhour[((current_time.hour % 12) * 4) + (current_time.minute / 15)], 255);
  step_minute = led_sequencer_sequence_push_back_step("m", LED_SEQUENCER_STEP_SHOW, "c", led_mapping_minute[current_time.minute], 255);
  step_second = led_sequencer_sequence_push_back_step("s", LED_SEQUENCER_STEP_SHOW, "c", led_mapping_minute[current_time.second], 255);

  enqueue_hourly_show();

  led_sequencer_run();

  srand(current_time.hour * current_time.minute * current_time.second);

  /*
   * Main infinite loop.
   */
  while(1)
  {
    /* Handle UART input when ready. */
    if(ready_flags & READY_UART_DATA)
    {
      ready_flags &= ~READY_UART_DATA;
      handle_uart_input(u0);
    }

    /* A time change has occurred, update the clock display. */
    if(ready_flags & READY_UPDATE_HMS)
    {
      ready_flags &= ~READY_UPDATE_HMS;
      update_hms();
    }

    /*
     * Run one loop through the current sequence, with interrupts disabled.
     */
    cli();
    led_sequencer_display();
    sei();
  }

  /* Never reached. */
  return(0);
}
