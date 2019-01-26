/* tjs_temp.h - access AVR on-chip temperature sensor.
 *
 * These functions read the on-chip temperature sensor.  The code
 * reads the sensor calibration data from the factory signature row
 * of the EEPROM.
 *
 * Copyright (C) Timothy J. Salo, 2018.
 */

float readTemperatureSensor(void);       // read on-chip temperature sensor
