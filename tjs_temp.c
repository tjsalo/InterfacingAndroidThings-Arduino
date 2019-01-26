/* tjs_temp.c - read on-chip temperature sensor.
 *
 * This code also reads the factory signature row to get the the
 * factory on-chip temperture sensor calibration data.
 *
 * Copyright (C) Timothy J. Salo, 2018.
 */

#include <stdio.h>
 
#include <avr/boot.h>

#include "simpleSerial.h"
#include "tjs_adc.h"


/* CPU on-chip temperature sensor factory calibration data locations.
 * TEMPSENSE0 and TEMPSENSE1 contain the temperature sensor value 
 * measured at the factory at HOTTEMP.  TEMPSENS2 and TEMPSENS3 contain
 * the temperature sensor value measured in the factory at ROOMTEMP.
 * These bytes are held in the EEPROM.
 * 
 */

#define ROOMTEMP 0x1e
#define HOTTEMP 0x1f
#define TEMPSENSE2 0x2c
#define TEMPSENSE3 0x2d
#define TEMPSENSE0 0x2e
#define TEMPSENSE1 0x2f

/* Temperature sensor channel. */

#define TEMPCHANNEL 0b100111

static unsigned char calibrationData[4];

static int initialized = 0;
static int temperatureCalibrationValid = 0;


void readTempCalibrationData();			// doesn't get anything for this processor



/* readTemperatureSensor() - read CPU on-chip temperature sensor.
 *
 * This function reads the on-chip temperature sensor calibration data,
 * if it has not been read already. 
 */
 
float readTemperatureSensor(void) {
	
	int temperature;
	
	/* Read factory calibration data, if necessary. */
	
	if (!initialized) {
		readTempCalibrationData();
		initialized = 1;
	}

	/* Read on-chip temperature sensor. */
	
	initAdc();                          // FIXME: shouldn't have to do every time
    readAdc(TEMPCHANNEL);               // read temp sensor first time
    initAdc();                          // FIXME: shouldn't have to do every time
    temperature =  readAdc(TEMPCHANNEL);    // read temp sensor second time

    /* Constants copied from http://microchipdeveloper.com/8avr:avradc
     * No explanation provided about their derivation. */

	return (temperature - 247.0)/1.22;
}



/* readTempCalibrationData - read on-chip temperature calibration data.
 * Note: not all AVR chips have factory temperature sensor calibration data.
 * Note: the ATmega32U4 AVR chip does _not_ have any factory temperature
 * sensor calibration data.
 */
 
void readTempCalibrationData() {

#define LOTNUM0 0x08	
#define LOTNUM1 0x09
#define LOTNUM2 0x0a
#define LOTNUM3 0x0b
#define LOTNUM4 0x0c
#define LOTNUM5 0x0d

    uint8_t sr[12] = {0xcc};

    sr[0] = boot_signature_byte_get(ROOMTEMP);
    sr[1] = boot_signature_byte_get(HOTTEMP);
    sr[2] = boot_signature_byte_get(TEMPSENSE0);
    sr[3] = boot_signature_byte_get(TEMPSENSE1);
    sr[4] = boot_signature_byte_get(TEMPSENSE2);
    sr[5] = boot_signature_byte_get(TEMPSENSE3);
    sr[6] = boot_signature_byte_get(LOTNUM0);
    sr[7] = boot_signature_byte_get(LOTNUM1);
    sr[8] = boot_signature_byte_get(LOTNUM2);
    sr[9] = boot_signature_byte_get(LOTNUM3);
    sr[10] = boot_signature_byte_get(LOTNUM4);
    sr[11] = boot_signature_byte_get(LOTNUM5);

//    printf("ROOMTEMP: %x, %d\n", sr[0], sr[0]);
//    printf("HOTTEMP: %x, %d\n", sr[1], sr[1]);
//    printf("TEMPSENSE0, TEMPSENSE1: %x, %x, %d\n", sr[2], sr[3],
//        ((sr[3] << 8) | sr[2]));
//    printf("TEMPSENSE2, TEMPSENSE3: %x, %x, %d\n", sr[4], sr[5],
//        ((sr[5] << 8) | sr[4]));
//    printf("Lot: %s%s%s%s%s%s\n", sr[6], sr[7], sr[8], sr[9], sr[10], sr[11]);
//  
//    printf("Device signature bytes: %x, %x, %x, %x\n",
//        boot_signature_byte_get(0), boot_signature_byte_get(1),
//	    boot_signature_byte_get(2), boot_signature_byte_get(3));
}
