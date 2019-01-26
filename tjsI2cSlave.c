/* tjsI2cSlave.c - implement I2C slave.
 *
 * This code supports Slave Receive (SR) and Slave Transmit (ST) modes
 * on the AVR Two Wire Interface (TWI), also known as the I2C protocol.
 *
 * Loosely based on “AVR I2C Slave Code” by GitHub user thegouger.
 * <https://github.com/thegouger/avr-i2c-slave>
 *
 * Copyright (C) Timothy J. Salo, 2018.
 */
 
#include <string.h>

#include <util/twi.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "tjs_leds.h"
#include "tjsI2cSlave.h"

/* Enable / Disable debug printing. */

#define RX_DEBUG 0
#define TX_DEBUG 1


/* State of I2C/TWI link. */

enum State {IDLE, HELLO_SENT, LINK_ESTABLISHED, SEND_SENT, LINK_ACTIVE};
enum State state;

/* I2C receive processing. */

#define I2C_RX_BUFFER_LENGTH 100

volatile unsigned char i2cRxBuffer[I2C_RX_BUFFER_LENGTH];  // buffer containing I2C command
volatile int i2cRxBufferp = 0;                // pointer into i2cRxBuffer
volatile unsigned int i2cCommandReady = 0;    // set if i2c command received

/* I2C transmit processing. */

#define I2C_TX_BUFFER_LENGTH 100

volatile unsigned char i2cTxBuffer[I2C_TX_BUFFER_LENGTH];  // buffer containing I2C command
volatile int i2cTxBufferp = 0;                 // pointer into i2cTxBuffer
volatile int i2cTxBufferLock = 0;		// lock on tx buffer
volatile int i2cTransmitSensorData = 0;	// to send temperature sensor readings

/* Debug print buffer. */

extern unsigned char debugBuffer[500];
extern int debugCommandReady;
char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

extern char tempString[];				// Latest temp reading "temp: nn.n"


/* tjsI2cInit - initialze TWI (I2C) interface.
 */
 
void tjsI2cInit(uint8_t address) {

    unsigned char sreg;                 // save status register (interrupt state)
	
	sreg = SREG;                        // save current status
	cli();

	TWAR = address << 1;				// set I2C slave address, ignore general call addr
	TWCR = 0;							// set up TWI control register
	TWCR |= 1 << TWIE;					// TWI (I2C) interrupt enable
	TWCR |= 1 << TWEA;					// TWI enable acknowledgement
	TWCR |= 1 << TWINT;					// TWI enable interrupts
	TWCR |= 1 << TWEN;					// TWI enable
	
	DDRD &= ~(1 << 0);					// set pull-up resistors on SCL, SDA
	DDRD &= ~(1 << 1);
	PORTD |= (1 << 0);
	PORTD |= (1 << 1);
	
	i2cTxBufferp = 0;					// initialize receive buffer next in

	SREG = sreg;						// restore interrupt state
}



/* tjsI2cStop - stop I2C interface.
 */
 
void tjsI2cStop(void) {
    cli();
    TWCR = 0;							// clear ...
    TWAR = 0;							// clear ...
    sei();
}



/* ISR(TWI_vect) - process TWI (I2C) interrupts.
 */
 
ISR(TWI_vect) {

    /* Process based on TWI status.
	 *
     * Note:  most comments directly from AVR datasheet and twi.h.
	 */
		
    switch(TW_STATUS) {					// TWSR, status bits only
		
		unsigned char ch;				// used by debug
		char chars[8];					// "ff (c)\n\0"
		
		/* Slave Receive - Receive data byte from master (and ACK returned).
         * (TW_SR_DATA_ACK - 0x80)
		 */
		 
		char tempBuffer[50];

        case TW_SR_DATA_ACK:
			displayOctalDigit(1);
			if (i2cRxBufferp == I2C_RX_BUFFER_LENGTH) i2cRxBufferp--;	// avoid buffer overrun
            i2cRxBuffer[i2cRxBufferp++] = TWDR;
			i2cRxBuffer[i2cRxBufferp] = '\0';
			if (i2cRxBuffer[i2cRxBufferp-1] == 0x0a) {    // if '\n'
				i2cRxBuffer[--i2cRxBufferp] = '\0';		// eat '\n'
				i2cCommandReady = 1;
			}
            TWCR = (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
			
			/* Debug */
			
			if (RX_DEBUG) {
				if (i2cCommandReady != 1) {
					ch = toascii(i2cRxBuffer[i2cRxBufferp-1]);
					if (iscntrl(ch)) ch = '.';
					if (i2cRxBufferp == 1) {
						strcpy(debugBuffer, "\nTW_SR_DATA_ACK: ");
					} else {
						strlcat(debugBuffer, "TW_SR_DATA_ACK: ", 500);
					} 
					chars[0] = hex[(i2cRxBuffer[i2cRxBufferp-1] >> 4) & 0xf];
					chars[1] = hex[i2cRxBuffer[i2cRxBufferp-1] & 0xf];
					chars[2] = ' ';
					chars[3] = '\'';
					chars[4] = ch;
					chars[5] = '\'';
					chars[6] = '\n';
					chars[7] = '\0';
					strlcat(debugBuffer, chars, 500);
				} else {
					debugCommandReady = 1;
				}
			}
            break;
			
		/* Slave Receive - Received data byte from master (and NACK returned).
         * (TW_SR_DATA_NACK - 0x88)
		 *
		 * This should never be received with this code.
		 */
		
        case TW_SR_DATA_NACK:
			displayOctalDigit(2);
			if (i2cRxBufferp == I2C_RX_BUFFER_LENGTH) i2cRxBufferp--;	// avoid buffer overrun
            i2cRxBuffer[i2cRxBufferp++] = TWDR;
			i2cRxBuffer[i2cRxBufferp] = '\0';
			if (i2cRxBuffer[i2cRxBufferp-1] == 0x0a) {    // if '\n'
				i2cRxBuffer[--i2cRxBufferp] = '\0';		// eat '\n'
				i2cCommandReady = 1;
			}
            TWCR = (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
			if (RX_DEBUG) {
				if (i2cCommandReady != 1) {
					ch = toascii(i2cRxBuffer[i2cRxBufferp-1]);
					if (iscntrl(ch)) ch = '.';
					if (i2cRxBufferp == 1) {
						strcpy(debugBuffer, "\nTW_SR_DATA_NACK: ");
					} else {
						strlcat(debugBuffer, "TW_SR_DATA_NACK: ", 500);
					} 
					chars[0] = hex[(i2cRxBuffer[i2cRxBufferp-1] >> 4) & 0xf];
					chars[1] = hex[i2cRxBuffer[i2cRxBufferp-1] & 0xf];
					chars[2] = ' ';
					chars[3] = '\'';
					chars[4] = ch;
					chars[5] = '\'';
					chars[6] = '\n';
					chars[7] = '\0';
					strlcat(debugBuffer, chars, 500);
				} else {
					debugCommandReady = 1;
				}
			}
            break;
			
		/* Slave Transmit - Own SLA+R has been received; ACK has been returned
         * (TW_ST_SLA_ACK - 0xA8)
		 */
		
        case TW_ST_SLA_ACK:
		    // receive this..
			displayOctalDigit(3);
			if (i2cTxBuffer[i2cTxBufferp] != 0) {
				TWDR = i2cTxBuffer[i2cTxBufferp++];    // transmit next byte
				TWCR = (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
			} else {
				TWDR = 0;
				TWCR = (1<<TWIE) | (1<<TWINT) | (1<<TWEN);
			}
			if (TX_DEBUG) {
				ch = toascii(i2cTxBuffer[i2cTxBufferp-1]);
				if (iscntrl(ch)) ch = '.';
				if (i2cTxBufferp == 1) {
					strcpy(debugBuffer, "\nTW_ST_SLA_ACK: ");
				} else {
					strlcat(debugBuffer, "TW_ST_SLA_ACK: ", 500);
				} 
				chars[0] = hex[(i2cTxBuffer[i2cTxBufferp-1] >> 4) & 0xf];
				chars[1] = hex[i2cTxBuffer[i2cTxBufferp-1] & 0xf];
				chars[2] = ' ';
				chars[3] = '\'';
				chars[4] = ch;
				chars[5] = '\'';
				chars[6] = '\n';
				chars[7] = '\0';
				strlcat(debugBuffer, chars, 500);
			}
			break;
	  
	    /* Slave Transmit - Data byte in TWDR has been transmitted; ACK has been received
		 *(TW_ST_DATA_ACK - 0xB8)
		 */
		
		case TW_ST_DATA_ACK:
		    // receive this...
			displayOctalDigit(4);
			if (i2cTxBuffer[i2cTxBufferp] != 0) {
				TWDR = i2cTxBuffer[i2cTxBufferp++];    // transmit next byte
				TWCR = (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
			} else {
				TWCR = (1<<TWIE) | (1<<TWINT) | (1<<TWEN);
				TWDR = 0;
			}
			if (TX_DEBUG) {
				ch = toascii(i2cTxBuffer[i2cTxBufferp-1]);
				if (iscntrl(ch)) ch = '.';
				if (i2cTxBufferp == 1) {
					strcpy(debugBuffer, "\nTW_ST_DATA_ACK: ");
				} else {
					strlcat(debugBuffer, "TW_ST_DATA_ACK: ", 500);
				} 
				chars[0] = hex[(i2cTxBuffer[i2cTxBufferp-1] >> 4) & 0xf];
				chars[1] = hex[i2cTxBuffer[i2cTxBufferp-1] & 0xf];
				chars[2] = ' ';
				chars[3] = '\'';
				chars[4] = ch;
				chars[5] = '\'';
				chars[6] = '\n';
				chars[7] = '\0';
				strlcat(debugBuffer, chars, 500);
				debugCommandReady = 1;
			}
			break;

		/* Slave Transmit - Data byte in TWDR has been transmitted; NOT ACK has been received
         * (TW_ST_DATA_NACK - 0xC0)
		 */
		 
		case TW_ST_DATA_NACK:
		    // receive this...
			displayOctalDigit(5);
			TWCR = (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
			if (TX_DEBUG) {
				ch = toascii(i2cTxBuffer[i2cTxBufferp-1]);
				if (iscntrl(ch)) ch = '.';
				sprintf(tempBuffer, "TW_ST_DATA_NACK: %d \'%c\'\n",
						i2cTxBufferp-1, ch);	// ****** debug ******
				strcat(debugBuffer, tempBuffer);
			}
			break;

		/* Slave Transmit - Last data byte in TWDR has been transmitted (TWEA = “0”);
         * ACK has been received.
		 * (TW_ST_LAST_DATA - 0xC8)
		 */

		case TW_ST_LAST_DATA:
		    // receive this...
			displayOctalDigit(6);
			TWCR = (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
			if (TX_DEBUG) {
				ch = toascii(i2cTxBuffer[i2cTxBufferp-1]);
				if (iscntrl(ch)) ch = '.';
				sprintf(tempBuffer, "TW_ST_LAST_DATA: %i \'%c\'\n",
						i2cTxBufferp-1, ch);	// ****** debug ******
				strcat(debugBuffer, tempBuffer);
				debugCommandReady = 1;
			}
			break;
			
		/* I2C bus error.
		 * Prepare to receive address again (i.e., wait for next message).
		 */
		
		case TW_BUS_ERROR:
//			displayOctalDigit(7);
            TWCR = 0;
			TWCR = (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN); 
			break;
	  
		/* Everything else.
		 * Prepare to receive address again (i.e., wait for next message).
		 */
		 
		default:
//			displayOctalDigit(7);
			TWCR = (1<<TWIE) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);
			break;
	}
} 


char* processI2cCommand() {

    char* token;						// pointer to current token
	char* pointer = i2cRxBuffer;		// used by strcmp_r
	static char string[30];				// returned string

	token = strtok_r(i2cRxBuffer, " ", &pointer);
	
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
