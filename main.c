/* CSCI 5143 Project.
 *
 * This code periodically reads the temperature internal to the AVR
 * processor and periodically transmits that data over the async, I2C,
 * and SPI interfaces.  This progam is expected to run with companion
 * code that runs on a Raspberry Pi.
 *
 * Copyright (C) Timothy J. Salo, 2018.
 */


#define F_CPU 16000000ul

#include <stdio.h>
#include <avr/boot.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

/* TJS includes. */

#include <errno.h>
#include <stdio.h>
#include "simpleSerial.h"
#include "tjs_adc.h"
#include "tjs_msec_clock.h"
#include "tjs_temp.h"
#include "tjsI2cSlave.h"
#include "tjsSpiSlave.h"
//#include "cpu_clock.h"
//#include "tjs_leds.h"


/* I2C Slave Address. */

#define I2C_ADDR 0x77


/* Define Interfaces */

#define INTERFACE_ASYNC 0
#define INTERFACE_I2C 1
#define INTERFACE_SPI


/* Forward References. */

char* processNullCommand(char *);
char* processPCommand(char *);
char* processHelloCommand(char *);
char* processOnOffCommand(int *, char *);



/* Global State. */

unsigned long int timeLast = 0;         // previous time
unsigned long int timeNow = 0;          // time of latest reading

/* User command input processing. */

extern  char recv_buffer[];                 // buffer containing user command
extern  uint8_t recv_buffer_ptr;            // pointer into recv_buffer
extern  unsigned int user_command_ready;    // set if user command received

/* Flags to control printing of state information and other activities. */

int printDetailedInfo = 1;              // enables periodic printing of globla state
int printFinegrainedInfo = 0;           // enables printing of fine-grained info
int readTempSensor = 1;                 // enables reading of temperature sensors

/* I2C  input processing. */

#define I2C_BUFFER_LENGTH 50

extern unsigned char i2cRxBuffer[I2C_RX_BUFFER_LENGTH];  // buffer containing I2C command
extern int i2cRxBufferp;                // pointer into i2CRecvBuffer
extern unsigned int i2cCommandReady;    // set if i2c command received
extern int i2cRxBufferpOld;	    		// DEBUG: old i2cRecvBufferp

/* I2C transmit buffer */

extern unsigned char i2cTxBuffer[I2C_TX_BUFFER_LENGTH];
extern int i2cTxBufferp;

/* I2C  input processing. */

#define SPI_BUFFER_LENGTH 50

extern unsigned char spiRxBuffer[SPI_RX_BUFFER_LENGTH];  // buffer containing SPI command
extern int spiRxBufferp;                // pointer into spiRecvBuffer
extern unsigned int spiCommandReady;    // set if spi command received
extern int spiRxBufferpOld;	    		// DEBUG: old spiRecvBufferp

/* SPI transmit buffer */

extern unsigned char spiTxBuffer[SPI_TX_BUFFER_LENGTH];
extern int spiTxBufferp;

/* Debug print buffer */

unsigned char debugBuffer[500];
int debugCommandReady = 0;

char tempString[20];


/****** main() ******/
  
int main(void) {
	
	USBCON = 0;

	/* Initialize everything. */
	
	cli();

	initAdc();                          // initialize ADC
	
	tjsI2cInit(I2C_ADDR);				// initialize I2C slave

	tjsSpiInit();						// initialize SPI slave

	/* Register command processors. */

    registerUserCommand("", processNullCommand);
    registerUserCommand("p", processPCommand);
	registerUserCommand("P", processPCommand);
	registerUserCommand("hello:", processHelloCommand);

	/* Blink red LED to confirm board booted up (and detect reboots). */

	enableRedLED();
	enableYellowLED();
	enableGreenLED();
	onRedLED();
	_delay_ms(500);
	offRedLED();
	_delay_ms(500);
	onRedLED();
	_delay_ms(500);
	offRedLED();

    /* print banner. */
	
    uart_init();

    stdout = &mystdout;					// make avr-libc functions work
    stdin  = &mystdin;

    printf("\n\n");
    printf("# Android Things / Arduino Integration\n");
    printf("# Timothy J. Salo\n\n");
	
	waitOutputComplete();

    /* Manage reading of on-chip temperature sensor. */
	
	unsigned long int tempNextTime = getMsecClock();    // last pot ADC time
	unsigned int tempPeriod = 100;      // read temp every 100 msec
	float tempLastValue = 9999.0f;      // last temp ADC value
	float tempCurrentValue = 0.0f;      // current temp ADC value

	/* Control printing of detailed state information. */
	
    unsigned long int printNextTime;    // next time to print
    unsigned int printPeriod = 1000;    // print state every second
	printNextTime = getMsecClock();     // set time for detailed info print

	/* Fire up millisecond clock. */
	
	initializeMsecClock();
	startMsecClock();

    /****** Main loop. ******/
	
	while(1) {

        /* Check for debug message.
		 * Note: this allows interrupt code to print something.
 		 */

		 if (debugCommandReady) {
			printf(debugBuffer);
			waitOutputComplete();
			cli();
            debugCommandReady = 0;
			sei();
        }
		
        /* Check for command on async interface. */
    
        if (user_command_ready) {
	        printf("rx: %s\n", recv_buffer);
			char *asyncString = processUserCommand(recv_buffer);
			if (asyncString != NULL) printf(asyncString);
            recv_buffer_ptr = 0;
            recv_buffer[recv_buffer_ptr] = '\0';
            user_command_ready = 0;
        }
		
        /* Check for input on I2C interface.
		 * Note: this allows command processing to run with interrupts enabled.
 		 */
    
        if (i2cCommandReady) {
			printf("I2C   rx: %s\n", i2cRxBuffer);
			char *i2cString = processI2cCommand(i2cRxBuffer);
			printf("I2C resp: %s\n", i2cString);
			if (i2cString != NULL) {
				strlcpy(i2cTxBuffer, i2cString, sizeof(i2cTxBuffer));
				i2cTxBufferp = 0;
			}
			cli();
            i2cRxBufferp = 0;
            i2cRxBuffer[i2cRxBufferp] = '\0';
            i2cCommandReady = 0;
			sei();
        }
		
        if (spiCommandReady) {
			printf("SPI   rx: %s\n", i2cRxBuffer);
			char *spiString = processSpiCommand(spiRxBuffer);
			printf("SPI resp: %s\n", spiString);
			if (spiString != NULL) {
				strlcpy(spiTxBuffer, spiString, sizeof(spiTxBuffer));
				spiTxBufferp = 0;
			}
			cli();
            spiRxBufferp = 0;
            spiRxBuffer[spiRxBufferp] = '\0';
            spiCommandReady = 0;
			sei();
        }
		
	    /* Read on-chip temperature sensor every 100 milliseconds, if enabled. */
		
		if (getMsecClock() >= tempNextTime) {
			if (readTempSensor) {
				tempCurrentValue = readTemperatureSensor();
                tempLastValue = tempCurrentValue;    // save current values
                tempNextTime = getMsecClock() + tempPeriod;
				sprintf(tempString, "temp: %4.1f\n", tempCurrentValue);
			}
        }
		
		/* Transmit detailed state information on ascyn interface every second. */
		
		if (getMsecClock() >= printNextTime) {
			if (printDetailedInfo) {
				printf(tempString);
		    }

            printNextTime = getMsecClock() + printPeriod;

        }
    }
}



/*****************************************************************************
 * Command processing                                                        *
 *****************************************************************************/


/* processHelloCommand - process "hello [<sequence>" command.  Respond with
 * an "ack [<sequence]".
 * Note: this code in _not_ reentrant.  Calling this funtion before the last
 * response it has returned has completed being output, may yield unpredictable
 * results.
 */

char* processHelloCommand(char *command) {
	
	static char string[30];
	strlcpy(string, "ack:", sizeof(string));
	char* token = strtok(NULL, " ");    // grab possible <sequence>
	if (token != NULL) {
		strlcat(string, " ", sizeof(string));
		strlcat(string, token, sizeof(string));
	}
	strlcat(string, "\n", sizeof(string));
	return string;
}
 

 /* processSCommand - process "P [on|off]" command to control printing of
 * state information.  An "P" command with no parameter toggles
 * the state of printing.
 */
 
char* processPCommand(char *command) {
	
	return processOnOffCommand(&printDetailedInfo, "Detailed info printing");
}


/* processNullCommand - return a list of commands in response to a null command. */

char* processNullCommand(char *command) {
    return "Enter a commmand:\n\n"
	       " p [on | off]    Toggle / enable / disable printing of state information\n"
		   " P [on | off]\n\n";
}


/* processOnOffCommand - process commands of the form: "cmd [on | off]"
 * Note that this fuction is not reentrant.  If it is called before the string
 * returned by the previous invocation has been completely output, unpredictable
 * results may occur.
 */
 
char* processOnOffCommand(int *flag, char *msg) {

	static char string[50];				// return value
	
	char* token = strtok(NULL, " ");    // grab possible "on" or "off"
	
	if ((token == NULL) || (strcmp(token, "") == 0)) {
		if (*flag) *flag = 0; else *flag = 1;    // toggle
	} else if (strcmp(token, "on") == 0) {
		*flag = 1;
	} else if (strcmp(token, "off") == 0) {
		*flag = 0;
	} else {

		strlcpy(string, "Unrecognized parameter: \"", sizeof(string));
		strlcat(string, token, sizeof(string));
		strlcat(string, "\"\n", sizeof(string));
		return string;
	}

	strlcpy(string, "****** ", sizeof(string));
	strlcat(string, msg, sizeof(string));
	if (*flag) {
		strlcat(string, " enabled ******\n", sizeof(string));
	} else {
		strlcat(string, " disabled ******\n", sizeof(string));
	}
	
    return string;
}
