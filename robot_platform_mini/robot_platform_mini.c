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
#include <math.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <uart.h>

typedef struct _pid_control_st
{
  float i_min;
  float i_max;
  float p_gain;
  float i_gain;
  float d_gain;
  float i_state;
  float d_state;
  float p_term;
  float i_term;
  float d_term;
} pid_control_t;

float update_pid(pid_control_t *pid, float error, float position)
{
  /* Set i_state and clamp to bounds. */
  pid->i_state += error;
  if(pid->i_state > pid->i_max)
    pid->i_state = pid->i_max;
  else if(pid->i_state < pid->i_min)
    pid->i_state = pid->i_min;

  /* Calculate PID terms. */
  pid->p_term = pid->p_gain * error;
  pid->i_term = pid->i_gain * pid->i_state;
  pid->d_term = pid->d_gain * (position - pid->d_state);

  /* Save last position in d_state. */
  pid->d_state = position;

  return pid->p_term + pid->i_term + pid->d_term;
}

uint16_t window_average(uint16_t old_average, uint16_t new_value, uint8_t size)
{
  uint32_t new_average = old_average;
  new_average *= (size - 1);
  new_average += new_value;
  new_average /= size;
  return (uint16_t) new_average;
}

volatile uint16_t time = 0;
volatile uint16_t a_time = 0, b_time = 0;
volatile uint8_t a_age = 0, b_age = 0;

ISR(TIMER0_COMPA_vect)
{
  time++;
  a_time++;
  b_time++;

  if((time % 10) == 0)
  {
    if(a_age < 255)
      a_age++;
    if(b_age < 255)
      b_age++;
  }
}

volatile uint16_t a_ticks = 0, a_tick_time = 0;
volatile uint16_t b_ticks = 0, b_tick_time = 0;

#define TICK_SLOTS 8

uint16_t average_times(volatile uint16_t tick_times[])
{
  uint8_t slot = 0;
  uint32_t sum = 0;

  for(slot=0; slot < TICK_SLOTS; slot++)
    sum += tick_times[slot];

  return sum / TICK_SLOTS;
}

volatile uint16_t a_tick_times[TICK_SLOTS];
volatile uint16_t b_tick_times[TICK_SLOTS];

ISR(INT0_vect) {
  PORTB |= _BV(PB2);

  a_tick_times[a_ticks++ % TICK_SLOTS] = a_time;
  a_tick_time = average_times(a_tick_times);
  a_time = 0;
  a_age  = 0;

  PORTB &= ~_BV(PB2);
}

ISR(INT1_vect) {
  PORTB |= _BV(PB3);

  b_tick_times[b_ticks++ % TICK_SLOTS] = b_time;
  b_tick_time = average_times(b_tick_times);
  b_time = 0;
  b_age  = 0;

  PORTB &= ~_BV(PB3);
}

float a_rev = 0, a_rev_target = 0.0;
float b_rev = 0, b_rev_target = 0.0;

void handle_uart_input(uart_t *uart)
{
  char c;

  c = uart_getc(uart);

  if ( c & UART_NO_DATA )
    return;

  switch(c)
  {
  case 'a':
    a_rev_target -= 0.1;
    break;
  case 'A':
    a_rev_target += 0.1;
    break;
  case 'b':
    b_rev_target -= 0.1;
    break;
  case 'B':
    b_rev_target += 0.1;
    break;
  case '0':
    a_rev_target = 0.0;
    b_rev_target = 0.0;
    break;
  case '1':
    a_rev_target = 1.0;
    b_rev_target = 1.0;
    break;
  case '2':
    a_rev_target = 2.0;
    b_rev_target = 2.0;
    break;
  case '3':
    a_rev_target = 3.0;
    b_rev_target = 3.0;
    break;
  case '4':
    a_rev_target = 4.0;
    b_rev_target = 4.0;
    break;
  case '5':
    a_rev_target = 5.0;
    b_rev_target = 5.0;
    break;
  case 'p':
    a_rev_target = 0.0;
    b_rev_target = 0.0;
    PORTA = _BV(PA0) | _BV(PA1) | _BV(PA2) | _BV(PA3);
    break;
  case 'f':
    PORTA = _BV(PA0) | _BV(PA2);
    break;
  case 'r':
    PORTA = _BV(PA1) | _BV(PA3);
    break;
  }
}

void control_motor(float rev, float rev_target, float pid_result, volatile uint8_t *ocr, volatile uint8_t *port, uint8_t bv0, uint8_t bv1)
{
  float ocr_new = (((float)*ocr) + pid_result);

  if(rev_target == 0.0 && rev == 0.0)
  {
    /* Short brake */
    *port |= bv0 | bv1;
    *ocr = 0;
    return;
  }

  if(rev_target > 0.0)
  {
    /* Go forwards */
    *port |= bv0;
    *port &= ~bv1;
  } else if(rev_target < 0.0) {
    /* Go backwards */
    *port &= ~bv0;
    *port |= bv1;
  }

  if(ocr_new < 0.0)
    *ocr = 0;
  else if(ocr_new > 255.0)
    *ocr = 255;
  else
    *ocr = (uint8_t) ocr_new;
}

/*

  PWMA = OC2A
  PWMB = OC2B
  AIN1 = PA0
  AIN2 = PA1
  BIN1 = PA2
  BIN2 = PA3

*/
int main(void)
{
  uart_t *u0;
  _delay_ms(1000);

  u0 = uart_init("0", 0);
  uart_init_stdout(u0);
  uart_set_rx_callback(u0, handle_uart_input);

  /* Trigger on the rising edge of INT{0,1} */
  EICRA = _BV(ISC11) | _BV(ISC10) | _BV(ISC01) | _BV(ISC00);
  /* Enable INT{0,1} interrupts */
  EIMSK = _BV(INT1) | _BV(INT0);

  /*
   * WGM01 = CTC mode
  */
  TCCR0A = _BV(WGM01);
  /* CS00 = Prescale by /8 */
  TCCR0B = _BV(CS01);
  TIMSK0 = _BV(OCIE0A);

  OCR0A = 100;

  /* Set up OC2A (PD7) and OC2B (PD6) as outputs. */
  DDRD |= _BV(PD6) | _BV(PD7);

  /* For debugging */
  DDRB |= _BV(PB0) | _BV(PB1) | _BV(PB2) | _BV(PB3);

  /*
    COM2A1 = Clear OC2A on compare match
    COM2B1 = Clear OC2B on compare match
    WGM21|WGM20 = Fast PWM
  */
  TCCR2A = _BV(COM2A1) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  /* CS20 = No prescale */
  TCCR2B = _BV(CS20);

  OCR2A = 0;
  OCR2B = 0;

  /* Set up AIN1,2 and BIN1,2 as outputs. */
  DDRA |= _BV(PA0) | _BV(PA1) | _BV(PA2) | _BV(PA3);

  sei();

  uart_puts(u0, "\r\n\r\nBooted up!\r\n");

  pid_control_t a_pid, b_pid;

  a_pid.i_min = -1.0;
  a_pid.i_max = +1.0;
  a_pid.i_state = 0.0;
  a_pid.p_gain = 2.0;
  a_pid.i_gain = 0.5;
  a_pid.d_gain = 2.0;

  b_pid.i_min = -1.0;
  b_pid.i_max = +1.0;
  b_pid.i_state = 0.0;
  b_pid.p_gain = 2.0;
  b_pid.i_gain = 0.5;
  b_pid.d_gain = 2.0;

  printf("a_r a_rt a_ocr a_pid a_t b_r b_rt b_ocr b_pid b_t a_b_t\n");
  printf("%4.2f %4.2f %3i %6.1f %5u %4.2f %4.2f %3i %6.1f %5u %5i\n",
    0.0, 0.0, 0, 0.0, 0,
    0.0, 0.0, 0, 0.0, 0,
    0);

  while(1)
  {
    PORTB |= _BV(PB0);
    if(a_age > 250)
      a_rev = 0.0;
    else
      a_rev = (1.0 / ((((float)a_tick_time)/10000.0) * 12.0));

    if(b_age > 250)
      b_rev = 0.0;
    else
      b_rev = (1.0 / ((((float)b_tick_time)/10000.0) * 12.0));

    float a_pid_result = update_pid(&a_pid, abs(a_rev_target) - a_rev, a_rev);
    float b_pid_result = update_pid(&b_pid, abs(b_rev_target) - b_rev, b_rev);

    control_motor(a_rev, a_rev_target, a_pid_result, &OCR2A, &PORTA, _BV(PA0), _BV(PA1));
    control_motor(b_rev, b_rev_target, b_pid_result, &OCR2B, &PORTA, _BV(PA2), _BV(PA3));

    if(1 || (a_rev_target != 0.0 || b_rev_target != 0.0))
    {
      printf("%4.2f %4.2f %3i %6.1f %5u %4.2f %4.2f %3i %6.1f %5u %5i\n",
        (double)a_rev, (double)a_rev_target, OCR2A, (double)a_pid_result, a_ticks,
        (double)b_rev, (double)b_rev_target, OCR2B, (double)b_pid_result, b_ticks,
        (a_ticks - b_ticks));
    }
    PORTB &= ~_BV(PB0);

    while(time < 1000) {
      _delay_us(10);
    }
    cli();
    time = 0;
    sei();
  }

  return(0);
}
