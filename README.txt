The following files comprise the Arduino portion of the software for this 
project.  All of this code was written by the author, except as noted.  Some 
snippets of code were copied more or less directly from the AVR datasheet.

main.c

main.c loops continuously, checking for any command that might be received 
over one of the interfaces, and periodically reading the on-chip temperature 
sensor and transmitting temperature sensor data over the interfaces.

A minimal protocol to control communications between the Arduino and 
Raspberry Pi was partially implemented.  At this point, the Arduino software 
responds to a “hello: <sequence>” command with “ack: <sequence>”.  The 
software also periodically transmits “temp: <value>” responses to the 
Raspberry Pi.

I2CSlave.c

I2CSlave implements a driver for an I2C slave.  This code is loosely based on 
“AVR I2C Slave Code” by GitHub user thegouger. 
<https://github.com/thegouger/avr-i2c-slave>.  The original thegouger code 
has been heavily modified, and future versions of this driver will likely 
remove the dependence on the thegouger code.

The I2CSlave.h software is currently able to receive multi-byte messages, but 
only transmits single-byte messages.  This limitation is being remedied.

SimpleSerial.c

SimpleSerial.c is an interrupt-driven async driver, which is based on code 
developed earlier in the semester.  It was modified to clean up the interface 
between the caller and the driver.  The command processing currently embedded 
in SimpleSerial.c needs to be refactored into a separate file.  Currently, 
this command processor has the unfortunate side-effect that responses to 
commands are always transmitted over the async interface.

tjs_adc.c

tjs_adc.c is a driver for the AVR analog-to-digital converter (ADC).  
Developed earlier in the semester, this code was modified to use the internal 
2.56 Volt reference (as required by the temperature sensor), rather than VCC.

tjs_leds.c

tjs_leds.c is a driver for on-board and GPIO-connected LEDs.  It has the 
perceived benefit that the user does not need to know and remember the ports, 
registers, and bits associated with each LED.

tjs_msec_clock.c

tjs_msec_clock.c implements a millisecond clock using timer4.  An important 
objective of this code was to use a timer that other code was unlikely to 
use.

tjs_temp.c

the_temp.c reads the on-chip temperature sensor and converts the sensor 
reading to Celsius.

This driver also reads the factory sensor calibration data from the factory 
signature row in EEPROM.  However, because the 32U4 doesn’t have any 
temperature sensor calibration data, this information isn’t actually used.
