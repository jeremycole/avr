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

/*

  Sequence 0
    Step 0
      type
      matrix, matrix_index
      ticks_remaining
      next
    ...
    Step N
    NULL
  ...
  Sequence N
  NULL

*/

#ifndef LED_SEQUENCER_H
#define LED_SEQUENCER_H

#include <inttypes.h>

#include <led_charlieplex.h>

/**
 * An LED matrix registered by name with the led_sequencer library.
 * Currently this only support the led_charlieplex library, but in the future
 * this should be extended to work with some others, in which case the main
 * charlieplex pointer will be renamed and changed to a void *.
 */
typedef struct _led_matrix_t
{
  char *name;
  led_charlieplex_t *charlieplex;
  struct _led_matrix_t *next;
} led_matrix_t;

/**
 * Different types of steps, which primarily allows for inserting blank delays
 * into sequences.
 */
typedef enum _led_sequence_step_type_t
{
  LED_SEQUENCER_STEP_DELAY_MS,
  LED_SEQUENCER_STEP_SHOW,
  LED_SEQUENCER_STEP_UNKNOWN = 255
} led_sequence_step_type_t;


#define LED_SEQUENCER_JIT_CONTINUE    1
#define LED_SEQUENCER_JIT_DEQUEUE     2
#define LED_SEQUENCER_JIT_INITIAL   100
#define LED_SEQUENCER_JIT_EMPTY     101
#define LED_SEQUENCER_JIT_ERROR     255

//typedef struct _led_sequence_step_t;
struct _led_sequence_step_t;
typedef uint8_t (led_sequencer_jit_function_t)(struct _led_sequence_step_t *step, uint8_t status) ;

/**
 * A single step and structure for singly linked list of steps within a
 * sequence.  Each step is executed for 'ticks_remaining' ticks of the global
 * sequencer clock.
 */
typedef struct _led_sequence_step_t
{
  led_sequence_step_type_t type;
  led_matrix_t *matrix;
  uint8_t matrix_index;
  uint8_t ticks_remaining;
  led_sequencer_jit_function_t *jit_function;
  struct _led_sequence_step_t *next;
} led_sequence_step_t;

/**
 * A sequence, registered by name and containing a singly linked list of
 * sequencer steps.
 */
typedef struct _led_sequence_t
{
  char *name;
  led_sequence_step_t *step;
  struct _led_sequence_t *next;
} led_sequence_t;

typedef enum _led_sequencer_status_t
{
  LED_SEQUENCER_HALTED = 0,
  LED_SEQUENCER_RUNNING = 1,
} led_sequencer_status_t;

/**
 * A global structure containing the two main items: all registered matrixes
 * and all registered sequences.  Due to its manipulation within ISRs, this
 * is only really useful as a global.
 */
typedef struct _led_sequencer_t
{
  led_matrix_t *matrix;
  led_sequence_t *sequence;
  led_sequencer_status_t status;
} led_sequencer_t;

/**
 * A global structure holding all sequencer-related data.
 */
extern led_sequencer_t *sequencer_global;

extern led_sequencer_t *led_sequencer_init(uint16_t hz);

extern void led_sequencer_halt();
extern void led_sequencer_run();

extern void led_sequencer_tick();
extern void led_sequencer_display();
extern void led_sequencer_dump_sequence();
extern void led_sequencer_timer_init(uint16_t hz);

extern led_matrix_t *led_sequencer_find_matrix_by_name(char *matrix_name);
extern led_matrix_t *led_sequencer_push_front_matrix(char *matrix_name, led_charlieplex_t *charlieplex);

extern led_sequence_t *led_sequencer_find_sequence_by_name(char *sequence_name);
extern led_sequence_t *led_sequencer_push_back_sequence(char *sequence_name);
extern void led_sequencer_sequence_push_back_jit(char *sequence_name, led_sequence_step_type_t type, char *matrix_name, led_sequencer_jit_function_t *jit_function);
extern led_sequence_step_t *led_sequencer_sequence_push_back_step(char *sequence_name, led_sequence_step_type_t type, char *matrix_name, char *led_name, uint8_t ticks);
extern void led_sequencer_sequence_modify_step(led_sequence_step_t *step, char *led_name, uint8_t ticks);
extern void led_sequencer_sequence_clear(char *sequence_name);

#endif /* LED_SEQUENCER_H */
