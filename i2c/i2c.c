/*
    Copyright (c) 2010, Jeremy Cole <jeremy@jcole.us>
    Copyright (c) 2006, Peter Fleury <pfleury@gmx.ch>

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

#include <stdlib.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <util/twi.h>

#include "i2c.h"

/* I2C clock in Hz */
#ifndef I2C_SCL_CLOCK
#define I2C_SCL_CLOCK  100000L
#endif

#define I2C_WAIT_CLEAR(v, b)  while(!((v) & _BV((b))))
#define I2C_WAIT_SET(v, b)    while((v) & _BV((b)))

#define I2C_ENABLE_ISR()      TWCR |= _BV(TWIE)
#define I2C_DISABLE_ISR()     TWCR &= ~_BV(TWIE)

volatile i2c_t i2c_global;

ISR(TWI_vect)
{
  uint8_t status;
  i2c_mode_t last_mode;

  last_mode = i2c_global.mode;
  status = TW_STATUS;

  /*
   * Receiving any of these statuses changes the current I2C mode regardless
   * of its current state.
   */
  switch(status)
  {
  case TW_ST_SLA_ACK:
  case TW_ST_ARB_LOST_SLA_ACK:
    i2c_global.mode = I2C_MODE_ST;
    break;
  case TW_SR_SLA_ACK:
  case TW_SR_ARB_LOST_SLA_ACK:
  case TW_SR_GCALL_ACK:
  case TW_SR_ARB_LOST_GCALL_ACK:
    i2c_global.mode = I2C_MODE_SR;
    break;
  case TW_SR_STOP:
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
    i2c_global.mode = I2C_MODE_IDLE;
    break;
  case TW_NO_INFO:
  case TW_BUS_ERROR:
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
    i2c_global.mode = I2C_MODE_IDLE;
    break;
  }

  switch(i2c_global.mode)
  {
  case I2C_MODE_MT:
  case I2C_MODE_MR:
    break;

  case I2C_MODE_ST:
    if(i2c_global.st_callback)
    {
      (*i2c_global.st_callback)(status, last_mode, i2c_global.mode);
      TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
    }
    break;
  case I2C_MODE_SR:
    if(i2c_global.sr_callback)
    {
      if(0 == (*i2c_global.sr_callback)(status, last_mode, i2c_global.mode))
        TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
    }
    break;

  case I2C_MODE_IDLE:
    if(last_mode == I2C_MODE_SR)
    {
      if(i2c_global.stop_callback)
      {
        (*i2c_global.stop_callback)(status, last_mode, i2c_global.mode);
      }
    }
    break;
  case I2C_MODE_UNKNOWN:
  default:
    break;
  }
}

/*
 * Initialization of the I2C bus interface.  Need to be called only once.
 */
void i2c_init(void)
{
  TWBR = ((F_CPU/I2C_SCL_CLOCK)-16)/2;
  i2c_global.mode = I2C_MODE_IDLE;
  i2c_global.st_callback = NULL;
  i2c_global.sr_callback = NULL;
  I2C_ENABLE_ISR();
}

uint8_t i2c_slave_init(uint8_t address, uint8_t address_mask, uint8_t gcall)
{
  TWAR  = address | (gcall & 0x01);
  TWAMR = address_mask;
  TWCR  = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);

  return TWAR;
}

/*
 * Issues a start condition and sends address and transfer direction.
 *
 * Return: 0 device accessible
 *         1 failed to access device
 */
uint8_t i2c_start(uint8_t address, uint8_t mode)
{
  uint8_t twst;

  i2c_global.mode = (mode == I2C_WRITE)?I2C_MODE_MT:I2C_MODE_MR;

  /* Send START condition */
  TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTA);

  /* Wait until transmission completed */
  I2C_WAIT_CLEAR(TWCR, TWINT);

  /* Check value of TWI Status Register. */
  twst = TW_STATUS;
  if ( (twst != TW_START) && (twst != TW_REP_START)) return twst;

  /* Send device address */
  TWDR = address | mode;
  TWCR = _BV(TWINT) | _BV(TWEN);

  /* Wait until transmission completed and ACK/NACK has been received */
  I2C_WAIT_CLEAR(TWCR, TWINT);

  /* Check value of TWI Status Register. */
  twst = TW_STATUS;
  if ( (twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK) ) return twst;

  return 0;
}


/*
 * Issues a repeated start condition and sends address and transfer direction.
 *
 * Input:   Address and transfer direction of I2C device.
 *
 * Return:  0 device accessible
 *          1 failed to access device
 */
uint8_t i2c_rep_start(uint8_t address, uint8_t mode)
{
  return i2c_start(address, mode);
}


/*
 * Terminates the data transfer and releases the I2C bus.
 */
void i2c_stop(void)
{
  /* Send STOP condition. */
  TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);

  /* Wait until STOP condition is executed and bus released. */
  I2C_WAIT_SET(TWCR, TWSTO);

  i2c_global.mode = I2C_MODE_IDLE;
  I2C_ENABLE_ISR();
}


/*
 * Send one byte to I2C device.
 *
 * Input:    byte to be transfered
 *
 * Return:   0 write successful
 *           1 write failed
*/
uint8_t i2c_write(uint8_t data)
{
  uint8_t twst;
    
  /* Send data to the previously addressed device */
  TWDR = data;
  TWCR = _BV(TWINT) | _BV(TWEN);

  /* Wait until transmission completed */
  I2C_WAIT_CLEAR(TWCR, TWINT);

  /* Check value of TWI Status Register. */
  twst = TW_STATUS;
  if(twst != TW_MT_DATA_ACK)
    return twst;

  return 0;
}


/*
 * Send a string of bytes to I2C device.
 *
 * Input:    byte to be transfered
 *
 * Return:   0 write successful
 *           1 write failed
*/
uint8_t i2c_write_array(uint8_t *data, uint8_t count)
{
  while(count--)
  {
    if(i2c_write(*data++) != 0) return count;
  }

  return 0;
}


/*
 * Read one byte from the I2C device, request more data from device.
 *
 * Return:  byte read from I2C device
 */
uint8_t i2c_read_ack(void)
{
  TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
  I2C_WAIT_CLEAR(TWCR, TWINT);

  return TWDR;
}


/*
 * Read one byte from the I2C device, read is followed by a STOP condition
 *
 * Return:  byte read from I2C device
 */
uint8_t i2c_read_nak(void)
{
  TWCR = _BV(TWINT) | _BV(TWEN);
  I2C_WAIT_CLEAR(TWCR, TWINT);

  return TWDR;
}


/*
 * Read multiple bytes from the I2C device, read may be followed by a STOP
 * condition if nak_last is true.
 *
 * Return:  zero
 */
uint8_t i2c_read_many(uint8_t *buffer, uint8_t count, uint8_t nak_last)
{
  while(count--)
  {
    *buffer++ = i2c_read(!(nak_last && (count==0)));
  }

  return 0;
}
