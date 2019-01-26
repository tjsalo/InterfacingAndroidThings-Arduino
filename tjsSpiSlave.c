/* tjsSpiSlave.c - implement SPI slave.
 *
 * This code supports ...
 *
 * Copyright (C) Timothy J. Salo, 2019.
 */
 
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "tjs_leds.h"
#include "tjsSpiSlave.h"

/* Enable / Disable debug printing. */

#define RX_DEBUG 1
#define TX_DEBUG 1


/* State of SPI link. */

enum State {IDLE, HELLO_SENT, LINK_ESTABLISHED, SEND_SENT, LINK_ACTIVE};
enum State state;

/* SPI receive processing. */

#define SPI_RX_BUFFER_LENGTH 100

volatile unsigned char spiRxBuffer[SPI_RX_BUFFER_LENGTH];  // buffer containing SPI command
volatile int spiRxBufferp = 0;                // pointer into spiRxBuffer
volatile unsigned int spiCommandReady = 0;    // set if SPI command received

/* SPI transmit processing. */

#define SPI_TX_BUFFER_LENGTH 100

volatile unsigned char spiTxBuffer[SPI_TX_BUFFER_LENGTH];  // buffer containing SPI command
volatile int spiTxBufferp = 0;          // pointer into spiTxBuffer
volatile int spiTxBufferLock = 0;		// lock on tx buffer
volatile int spiTransmitSensorData = 0;	// to send temperature sensor readings

/* Debug print buffer. */

extern unsigned char debugBuffer[500];
extern int debugCommandReady;
//char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

extern char tempString[];				// Latest temp reading "temp: nn.n"


/* tjsSpiInit - initialze SPI interface.
 * SPI Control Register – SPCR
 * SPI Status Register – SPSR
 * SPI Data Register – SPDR
 */
 
void tjsSpiInit() {

    unsigned char sreg;                 // save status register (interrupt state)
	
	sreg = SREG;                        // save current status
	cli();

	DDRB = (1 << DD3);					// set MISO to output
	SPCR = (1 << SPE);					// enable SPI
	SPCR ^= (1 << SPIE);				// enable interrupts
	
	unsigned char ch1 = SPSR;
	unsigned char ch2 = SPDR;
	
	spiTxBufferp = 0;					// initialize receive buffer next in
	spiRxBufferp = 0;					// initialize receive buffer next in

	SREG = sreg;						// restore interrupt state
}



/* tjsSpiStop - stop SPI interface.
 */
 
void tjsSpiStop(void) {
    cli();
    TWCR = 0;							// clear ...
    TWAR = 0;							// clear ...
    sei();
}

int count = 0;
unsigned char chOld;
unsigned char chNew;

/* ISR(SPI_STC_vect) - SPI Serial Transfer Complete interrupts.
 */
 
ISR(SPI_STC_vect) {
//	SPDR = 0xAA;
//	while(!(SPSR & (1 << SPIF)));   	// wait for data to be ready
//	enableYellowLED();
	toggleYellowLED();

	if (spiRxBufferp == SPI_RX_BUFFER_LENGTH) spiRxBufferp--;	// avoid buffer overrun
	
	if (SPDR == 0) return;
	
    spiRxBuffer[spiRxBufferp++] = SPDR;
	spiRxBuffer[spiRxBufferp] = '\0';
			if (spiRxBuffer[spiRxBufferp-1] == 0x0a) {    // if '\n'
				spiRxBuffer[--spiRxBufferp] = '\0';		// eat '\n'
				spiCommandReady = 1;
				strcpy(debugBuffer, spiRxBuffer);
				debugCommandReady = 1;
				spiRxBufferp = 0;
			}
			if (spiRxBufferp >= 20) {
				spiCommandReady = 1;
				strcpy(debugBuffer, spiRxBuffer);
				debugCommandReady = 1;
				spiRxBufferp = 0;
			}

	chOld = chNew;
	chNew = SPDR;
//	unsigned char ch = SPDR;
//	sprintf(debugBuffer, "SPI_STC_vect %i, %02x %02x\n", count++, chOld, chNew);
//	strcpy(debugBuffer, "SPI_SCT_vect\n");
//	debugCommandReady = 1;
}



char* processSpiCommand() {

    char* token;						// pointer to current token
	char* pointer = spiRxBuffer;		// used by strcmp_r
	static char string[30];				// returned string

	token = strtok_r(spiRxBuffer, " ", &pointer);
	
	/* Process "hello: <seq> command. */
	
	if (strcmp(token, "hello:") == 0) {
		
		strlcpy(string, "ack:", sizeof(string));
		char* token = strtok_r(NULL, " ", &pointer);    // grab possible <sequence>
		if (token != NULL) {
			strlcat(string, " ", sizeof(string));
			strlcat(string, token, sizeof(string));
		}
		strlcat(string, "\n", sizeof(string));
		return string;
	}

	/* Process "send: temp" command. */

	if (strcmp(token, "send:") == 0) {
		token = strtok_r(NULL, " ", &pointer);
		if ((token != NULL) & (strcmp(token, "temp") == 0)) {
			return tempString;
		} else {
			return "nack:\n";
		}
	}
	
	if (strcmp(token, "$:") == 0) {
		return NULL;
	}

	return "# Command not found\n";
	
}
