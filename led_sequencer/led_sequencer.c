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
#include <string.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include <led_charlieplex.h>

#include "led_sequencer.h"

led_sequencer_t *sequencer_global = NULL;

/**
 * Dump the current animation sequence as a series of Sequence and Step lines
 * using printf.  This is used only for debugging (mostly debugging the this
 * library itself).
 */
void led_sequencer_dump_sequence()
{
  led_sequence_t *sequence;
  led_sequence_step_t *step;

  for(sequence = sequencer_global->sequence; sequence; sequence = sequence->next)
  {
    printf_P(PSTR("Sequence 0x%04x, %s\n"),
      (uint16_t)sequence, sequence->name);
    for(step = sequence->step; step; step = step->next)
    {
      printf_P(PSTR("  Step 0x%04x, JIT 0x%04x, %i ticks\n"),
          (uint16_t)step, (uint16_t)step->jit_function, step->ticks_remaining);
    }
  }
}

/**
 * Interrupt handler for "tick" interrupt set up by led_sequencer_timer_init().
 */
ISR(TIMER0_COMPA_vect)
{
  led_sequencer_tick();
}

/**
 * Initialize Timer 0 to fire "hz" times per second.
 */
void led_sequencer_timer_init(uint16_t hz)
{
  /* Set pre-scaler to /256 */
  TCCR0B |= _BV(CS02);

  /* Force Output Compare A */
  TCCR0B |= _BV(FOC0A);

  /* Output Compare Interrupt Enable A */
  TIMSK0 |= _BV(OCIE0A);

  /* Set initial counter value */
  TCNT0 = 0;

  /* Set Output Compare Register to aim for 1ms interrupt */
  OCR0A = (F_CPU / 256) / hz ;
}

/**
 * Initialize the sequencer library, including the timer and memory structures.
 */
led_sequencer_t *led_sequencer_init(uint16_t hz)
{
  sequencer_global = malloc(sizeof(led_sequencer_t));

  led_sequencer_timer_init(hz);

  sequencer_global->matrix   = NULL;
  sequencer_global->sequence = NULL;
  sequencer_global->status   = LED_SEQUENCER_HALTED;

  return sequencer_global;
}

void led_sequencer_halt()
{
  if(sequencer_global)
    sequencer_global->status = LED_SEQUENCER_HALTED;
}

void led_sequencer_run()
{
  if(sequencer_global)
    sequencer_global->status = LED_SEQUENCER_RUNNING;
}

/**
 * For a given sequence step, if ticks_remaining has reached zero, free
 * the step.  Then, if the current step isn't perpetual, subtract a tick
 * from the step's ticks_remaining.
 */
void led_sequencer_tick_sequence_step(led_sequence_t *sequence)
{
  led_sequence_step_t *free_step;

  if(sequence->step->ticks_remaining == 0)
  {
    if(sequence->step->jit_function
        && ((*sequence->step->jit_function)(sequence->step, LED_SEQUENCER_JIT_EMPTY) == LED_SEQUENCER_JIT_CONTINUE)
        && (sequence->step->ticks_remaining > 0))
      goto refilled;

    free_step = sequence->step;
    sequence->step = sequence->step->next;
    free(free_step);

    if(sequence->step && sequence->step->jit_function)
      (*sequence->step->jit_function)(sequence->step, LED_SEQUENCER_JIT_INITIAL);
  }

  refilled:
  if(sequence->step && sequence->step->ticks_remaining != 0 && sequence->step->ticks_remaining != 0xff)
  {
    sequence->step->ticks_remaining--;
  }
}

/**
 * Iterate through all sequences in the global sequence list, for each of them,
 * call led_sequencer_tick_sequence_step.
 */
void led_sequencer_tick_sequence()
{
  led_sequence_t *sequence;
  uint8_t sequence_steps = 0;

  for(sequence = sequencer_global->sequence; sequence; sequence = sequence->next)
  {
    if(sequence->step != NULL)
    {
      led_sequencer_tick_sequence_step(sequence);
      sequence_steps++;
    }
  }
}

/**
 * Progress the animation sequence by one tick, often 1ms, but depends on
 * the value 'hz' passed to led_sequencer_init().  Normally called by the
 * Timer 0 Output Compare A interrupt (TIMER0_COMPA_vect).  This function
 * really just does the housekeeping and sanity checking and then calls out
 * to led_sequencer_tick_sequence() to do the real work.
 */
void led_sequencer_tick()
{
  TCNT0 = 0;

  if(sequencer_global && sequencer_global->status == LED_SEQUENCER_RUNNING)
  {
    led_sequencer_tick_sequence();
  }
}

/**
 * Play through one cycle of the LEDs which should currently be illuminated.
 * Normally this function is called during all spare time in the main loop,
 * and most other functions are called between runs of led_sequencer_run() or
 * through interrupts.
 */
void led_sequencer_display()
{
  led_sequence_t *sequence;

  if(sequencer_global->status != LED_SEQUENCER_RUNNING)
    return;

  for(sequence = sequencer_global->sequence; sequence; sequence = sequence->next)
  {
    if(sequence->step && sequence->step->type == LED_SEQUENCER_STEP_SHOW)
    {
      led_charlieplex_set_by_index(sequence->step->matrix->charlieplex, sequence->step->matrix_index);
      _delay_us(20);
      led_charlieplex_unset_last(sequence->step->matrix->charlieplex);
    }
  }

}

/**
 * Find a matrix in the global matrix list by its name.
 */
led_matrix_t *led_sequencer_find_matrix_by_name(char *matrix_name)
{
  led_matrix_t *matrix = sequencer_global->matrix;

  for(; matrix != NULL; matrix = matrix->next)
  {
    if(strcmp(matrix->name, matrix_name) == 0)
      return matrix;
  }

  return NULL;
}

/**
 * Push a matrix to the global matrix list.
 */
led_matrix_t *led_sequencer_push_front_matrix(char *matrix_name, led_charlieplex_t *charlieplex)
{
  led_matrix_t *matrix;

  if((matrix = malloc(sizeof(led_matrix_t))))
  {
    matrix->name = malloc(strlen(matrix_name)+1);
    strcpy(matrix->name, matrix_name);
    matrix->charlieplex = charlieplex;
    matrix->next = sequencer_global->matrix;
    sequencer_global->matrix = matrix;
  }

  return matrix;
}

/**
 * Find a sequence in the global sequence list by name.
 */
led_sequence_t *led_sequencer_find_sequence_by_name(char *sequence_name)
{
  led_sequence_t *sequence = sequencer_global->sequence;

  for(; sequence != NULL; sequence = sequence->next)
  {
    if(strcmp(sequence->name, sequence_name) == 0)
      return sequence;
  }

  return NULL;
}

/**
 * Add a sequence to the global sequence list, and return a pointer to it.
 */
led_sequence_t *led_sequencer_push_back_sequence(char *sequence_name)
{
  led_sequence_t *sequence, *last_sequence;

  if((sequence = malloc(sizeof(led_sequence_t))))
  {
    sequence->name = malloc(strlen(sequence_name)+1);
    strcpy(sequence->name, sequence_name);
    sequence->step = NULL;
    sequence->next = NULL;
  }

  if(sequencer_global->sequence)
  {
    for(last_sequence = sequencer_global->sequence; last_sequence->next; last_sequence = last_sequence->next);
    last_sequence->next = sequence;
  }
  else
  {
    sequencer_global->sequence = sequence;
  }

  return sequence;
}

void led_sequencer_sequence_push_back_jit(char *sequence_name, led_sequence_step_type_t type, char *matrix_name, led_sequencer_jit_function_t *jit_function)
{
  led_sequence_step_t *step;
  step = led_sequencer_sequence_push_back_step(sequence_name, type, matrix_name, NULL, 254);
  step->jit_function = jit_function;
  (*step->jit_function)(step, LED_SEQUENCER_JIT_INITIAL);
}

/**
 * Push a step on to the end of a sequence.
 */
led_sequence_step_t *led_sequencer_sequence_push_back_step(char *sequence_name, led_sequence_step_type_t type, char *matrix_name, char *led_name, uint8_t ticks)
{
  led_sequence_t *sequence;
  led_sequence_step_t *step, *last_step;

  if((step = malloc(sizeof(led_sequence_step_t))))
  {
    sequence = led_sequencer_find_sequence_by_name(sequence_name);

    step->type = type;
    step->matrix = led_sequencer_find_matrix_by_name(matrix_name);
    if(led_name)
    {
      step->matrix_index = led_charlieplex_find_index_by_name(step->matrix->charlieplex, led_name);
    }
    step->ticks_remaining = ticks;
    step->next = NULL;

    if(sequence->step)
    {
      for(last_step = sequence->step; last_step->next; last_step = last_step->next) {}
      last_step->next = step;
    }
    else
    {
      sequence->step = step;
    }

    return step;
  }

  return NULL;
}

/**
 * Modify a step in place within a sequence.  This is mostly useful for steps
 * with a 'ticks' value of 255 (never ending).
 */
void led_sequencer_sequence_modify_step(led_sequence_step_t *step, char *led_name, uint8_t ticks)
{
  step->matrix_index = led_charlieplex_find_index_by_name(step->matrix->charlieplex, led_name);
  step->ticks_remaining = ticks;
}

/**
 * Clear an entire sequence of any steps remaining.
 */
void led_sequencer_sequence_clear(char *sequence_name)
{
  led_sequence_t *sequence;

  sequence = led_sequencer_find_sequence_by_name(sequence_name);

  for(; sequence->step; sequence->step = sequence->step->next)
  {
    free(sequence->step);
  }
}
