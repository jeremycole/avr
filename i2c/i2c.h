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

#ifndef I2C_H
#define I2C_H

#include <util/twi.h>

/**@{*/

#if (__GNUC__ * 100 + __GNUC_MINOR__) < 304
#error "This library requires AVR-GCC 3.4 or later, update to newer AVR-GCC compiler !"
#endif

#include <avr/io.h>

/** defines the data direction (reading from I2C device) in i2c_start(),i2c_rep_start() */
#define I2C_READ    1

/** defines the data direction (writing to I2C device) in i2c_start(),i2c_rep_start() */
#define I2C_WRITE   0

/** Only the specific address will be responded to (rather than a range) */
#define I2C_ADDRESS_MASK_SINGLE 0

/** Disable responses on the general call address. */
#define I2C_GCALL_DISABLED 0

/** Enable responses on the general call address. */
#define I2C_GCALL_ENABLED  1

typedef enum _i2c_mode_t
{
  I2C_MODE_UNKNOWN = 0,
  I2C_MODE_IDLE,
  I2C_MODE_MT,
  I2C_MODE_MR,
  I2C_MODE_ST,
  I2C_MODE_SR
} i2c_mode_t;

typedef uint8_t (i2c_callback_t)(uint8_t status, i2c_mode_t last_mode, i2c_mode_t current_mode);

typedef struct _i2c_t
{
  i2c_mode_t mode;
  i2c_callback_t *st_callback;
  i2c_callback_t *sr_callback;
  i2c_callback_t *stop_callback;
} i2c_t;

extern volatile i2c_t i2c_global;

/**
 @brief initialize the I2C master interace. Need to be called only once 
 @param  void
 @return none
 */
extern void i2c_init(void);

extern uint8_t i2c_slave_init(uint8_t address, uint8_t address_mask, uint8_t gcall);

/** 
 @brief Terminates the data transfer and releases the I2C bus 
 @param void
 @return none
 */
extern void i2c_stop(void);


/** 
 @brief Issues a start condition and sends address and transfer direction 
  
 @param    addr address and transfer direction of I2C device
 @retval   0   device accessible 
 @retval   1   failed to access device 
 */
extern uint8_t i2c_start(uint8_t address, uint8_t mode);


/**
 @brief Issues a repeated start condition and sends address and transfer direction 

 @param   addr address and transfer direction of I2C device
 @retval  0 device accessible
 @retval  1 failed to access device
 */
extern uint8_t i2c_rep_start(uint8_t address, uint8_t mode);


/**
 @brief Issues a start condition and sends address and transfer direction 
   
 If device is busy, use ack polling to wait until device ready 
 @param    addr address and transfer direction of I2C device
 @return   none
 */
extern void i2c_start_wait(uint8_t address, uint8_t mode);

 
/**
 @brief Send one byte to I2C device
 @param    data  byte to be transfered
 @retval   0 write successful
 @retval   1 write failed
 */
extern uint8_t i2c_write(uint8_t data);

extern uint8_t i2c_write_array(uint8_t *data, uint8_t count);

/**
 @brief    read one byte from the I2C device, request more data from device 
 @return   byte read from I2C device
 */
extern uint8_t i2c_read_ack(void);

/**
 @brief    read one byte from the I2C device, read is followed by a stop condition 
 @return   byte read from I2C device
 */
extern uint8_t i2c_read_nak(void);

/** 
 @brief    read one byte from the I2C device
 
 Implemented as a macro, which calls either i2c_read_ack or i2c_read_nak
 
 @param    ack 1 send ack, request more data from device<br>
               0 send nak, read is followed by a stop condition 
 @return   byte read from I2C device
 */
extern uint8_t i2c_read(uint8_t ack);
#define i2c_read(ack)  ((ack) ? i2c_read_ack() : i2c_read_nak())

extern uint8_t i2c_read_many(uint8_t *buffer, uint8_t count, uint8_t nak_last);

/**@}*/
#endif
