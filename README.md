# Jeremy Cole's AVR Projects #

Many smaller projects written by Jeremy Cole on Atmel AVR MCUs:

* [uart](https://github.com/jeremycole/avr/tree/master/uart) -- A UART (serial) library based on Peter Fleury's uartlibrary. Also see [uart_test](https://github.com/jeremycole/avr/tree/master/uart_test).
* [i2c](https://github.com/jeremycole/avr/tree/master/i2c) -- An I2C (aka TWI) library initially based on Peter Fleury's i2cmaster. Now includes slave mode support using callback functions. Also see [i2c_master_test](https://github.com/jeremycole/avr/tree/master/i2c_master_test) and [i2c_slave_test](https://github.com/jeremycole/avr/tree/master/i2c_slave_test).
* [rtc](https://github.com/jeremycole/avr/tree/master/rtc) -- A custom real-time clock (RTC) library currently supporting the Maxim's DS1307 I2C-connected RTC chip. The rtc library requires the i2c library above. Also see [rtc_test](https://github.com/jeremycole/avr/tree/master/rtc_test).
* [lcd](https://github.com/jeremycole/avr/tree/master/lcd) -- An LCD library supporting both 8-bit and 4-bit parallel modes of the HD44780U LCD controller. Also see [lcd_test](https://github.com/jeremycole/avr/tree/master/lcd_test).
* [led_charlieplex](https://github.com/jeremycole/avr/tree/master/led_charlieplex) -- A custom library for generically describing the structure of and controlling a charlieplexed LED matrix.
* [led_sequencer](https://github.com/jeremycole/avr/tree/master/led_sequencer) -- A custom library for time-sequencing LED animations, supporting the led_charlieplex library for describing the LED matrix to play animations on.
* [nmea](https://github.com/jeremycole/avr/tree/master/nmea) -- An NMEA sentence parser for interoperation with GPS devices. Currently only supports `$GPRMC` sentences.

Larger specific projects:

* [led_analog_clock](https://github.com/jeremycole/avr/tree/master/led_analog_clock) -- The complete set of code using the uart (for debugging and setting the time), i2c, rtc, led_charlieplex, and led_sequencer libraries. Can connect to the lcd_i2c_digital_clock via I2C interface.
* [lcd_i2c_digital_clock](https://github.com/jeremycole/avr/tree/master/lcd_i2c_digital_clock) -- An LCD-based digital clock module that attaches to led_analog_clock via I2C interface and displays the time in digital form.
* [gps_i2c](https://github.com/jeremycole/avr/tree/master/gps_i2c) -- A translator from UART to I2C to allow a GPS to be attached to the led_analog_clock project to provide an accurate time sync source.