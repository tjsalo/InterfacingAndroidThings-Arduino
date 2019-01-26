/* simpleSerial - simple serial commmunications for AVR.
 *
 * simpleSerial enable libc I/O functions such as printf to use
 * the serial port on AVR chips.
 *
 * For an understanding of how this works, one should read the
 * comments in stdio.h.  And, you should look at the avr-libc
 * webpages:
 *
 * <https://www.nongnu.org/avr-libc/>
 *
 
 * A good example of the more advanced functionality that this approach
 * can yield can be found in the blog entry "Simple Serial Communications
 * With AVR libc" by Mika Tuupola, and the associated code.
 * 
 * <https://appelsiini.net/2011/simple-usart-with-avr-libc/>
 * <https://github.com/tuupola/avr_demo/tree/master/blog/simple_usart>
 *
 * Copyright (C) Timothy J. Salo, 2018.
 */
 
/* Current status.
 *
 * The transmit code appears to work, but could be improved.  Right now,
 * is does no buffering and doesn't use interrupts.
 *
 * I'm not exactly how the receive code ought to behave.  Should the
 * app simply block waiting for input? Should the app poll for received
 * data?  Should the app register a callback for received data?
 *
 * At the moment, the received code isn't assured to work.
 */
 
/* simpleSerial is known to work with the Adafruit Industries 954
 * serial interface.
 *
 * The short versions is: just use the Adafruit Industries 954
 * USB-to-TTL cable.
 *
 * The longer version is: I recommend this cable because:
 *
 * 1) It is known to work.  There are aleast a couple of common chips
 *    used in USB-to-Serial cables.  I haven't tested this code with
 *    anything other than the Adafruit cable.
 * 2) The Adafruit cable is a USB-to-TTL cable, not a USB-to-RS232
 *    cable.  TTL and RS-232 voltages are different.  I haven't tried
 *    to use an RS-232 cable with the AVR.  If you want to do this, you
 *    should probably read about level shifters.
 * 3) The Adafruit cable leaves the individual wires accessible on the
 *    non-USB end of the cable.  Many USB-to-Serial cables terminate in
 *    a connector, such as a DB-9.  This requires an extra step to connector
 *    it to the AVR.
 * 4) The Adafruit cable is relatively inexpensive; it is less expensive than
 *    many USB-to-Serial cables.
 *
 * Connecting the Adafruit 954 cable to your Arduino.
 *
 * Connect the black lead to a ground.
 * Connect the white lead (RX into USB port) to the AVR TX port (pin 1)
 * Connect the green lead (TX from the USB port) to the AVR RX port (pin 0)
 */
   
/* To use this code:
 *
 * Include in your source code:
 * 
 * #include "simpleSerial.h"
 *
 * Include in your source code directory:
 *
 * simpleSerial.c
 *
 * You will also need to include in your source code:
 *
 * #include "simpleSerial.h" 
 *
 * simpleSerial uses Port D, pin 2 for receive data and Port D, pin 3 for 
 * transmit data.
 *
 * Make sure your Makefile compiles all *.c files, not just main.c.
 *
 * By default, simpleSerial configures the serial interface to use:
 *
 * 8-bit characters
 * 1 stop bit
 * no party
 * 38,400 bps
 */
  
/* If you want to modify this code, or even just want to understand how is works, I recommend
 * reading the USART (Universal Synchronous and Asynchronous serial Receiver and Transmitter)
 * chapter of your favorite AVR datasheet.
 */
  
   
#define F_CPU 16000000ul
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <util/delay.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "simpleSerial.h"
#include "tjs_leds.h"

/* simpleSerial constants. */
// bit rate
#define SIMPLE_SERIAL_BIT_RATE 38400
// Transmit buffer size
#define XMIT_BUFER_SIZE 200


/* User command input processing. */

#define RECEIVE_BUFFER_LENGTH 50

volatile char recv_buffer[RECEIVE_BUFFER_LENGTH];    // does this need to be volatile?
volatile uint8_t recv_buffer_ptr = 0;                // does this neet to be volatile?
volatile unsigned int user_command_ready = 0;        // does this need to be volatile?



/* uart_init() - initialize USART port.
 */
 
void uart_init() {
    
    unsigned char sreg;                 // save status register (interrupt state)
	
	sreg = SREG;                        // save current status
	cli();

    UCSR1A =  0;                        // clear USART control registers
    UCSR1B =  0;
    UCSR1C =  0;

	/* Note: I can't get my ThinkPad / Adafruit 954 USB-TTL cable / A-Star
     * to run faster than 57600 bps. */
	 
//    UBRR1 = ((F_CPU/(16L*38400L)) - 1);    // set bit rate
//    UBRR1 = ((F_CPU/16)/115200L - 1); // set bit rate
//    UBRR1 = ((F_CPU/(16L*115200L)) - 1);   // set bit rate
//    UBRR1 = (F_CPU/115200L/16L - 1);  // set bit rate
    UBRR1 = ((F_CPU/16)/57600 - 1);   	// set bit rate
    UCSR1C |= (1 << UCSZ11) | (1 << UCSZ10);    // 8 bit char size
    UCSR1B |= (1 << TXEN1);     		// enable transmit
    UCSR1B |= (1 << RXEN1);     		// enable receive
    UCSR1B |= (1 << RXCIE1);     		// enable interrupt on data receipt
	
	SREG = sreg;						// restore interrupt state
}



/* Transmit buffer.
 * This buffers characters being transmitted.  As long as the buffer is not
 * full, programs should be able print to stdout without spin loops that wait
 * for the USART data register to be empty.  Rather uart_putchar employs
 * a spin lock only to wait for the buffer to be not full (specifically to
 * wait for one character to available to accept transmitted data).
 *
 * Only uart_putchar manipulates "in" and only uart_xmitchar manipulates "out".
 *
 * uart_xmitchar is largely driven by xxx interrupts.  uart_putchar briefly
 * locks out interrupts so that uart_xmitchar doesn't empty the buffer ...
 *
 * The buffer is a circular buffer.  It is empty when in = out;
 * it is full when in + 1 mod XMIT_BUFER_SIZE = out.
 */
 
static unsigned char xmitBuffer[XMIT_BUFER_SIZE] = {0};    // transmit buffer
volatile int in = 0;                    // next buffer slot to be filled
volatile int out = 0;                   // next buffer slot to be removed

static int spinLoops = 0;               // count of uart_putchar waits



/* uart_putchar() - write one character to USART port.
 */
 
int uart_putchar(char c, FILE *stream) {
    
    /* insert a '\r' before every '\n'.
     * Many terminal programs expect this.  However, this behavior breaks
     * binary transfers.  It would be nice if this behavior were controlled
     * by a run-time flag.
     */

    if (c == '\n') uart_putchar('\r', stream);    // insert \r prior to \n
    
    while (((in+1) % XMIT_BUFER_SIZE) == out) {   // spin waiting for buffer to empty
	    toggleRedLED();
	}
//    offRedLED();
	
    /* Insert character in buffer. */
    
    int sreg = SREG;                    // save interrupt state
    cli();
    xmitBuffer[in] = c;                 // insert character in circular buffer
    in = (in + 1) % XMIT_BUFER_SIZE;    // update in
    
    /* ****** DEBUG ****** */
    /* Print without interrupts. */
//	while (in != out) {                 // while buffer not empty
//		while (!(UCSR1A & (1 << UDRE1)));   // wait for Data Register to be empty
//		UDR1 = xmitBuffer[out];
//		out = (out + 1) % XMIT_BUFER_SIZE;
//	}
//    while (!(UCSR1A & (1 << UDRE1)));   // wait for Data Register to be empty
//
//    SREG = sreg;
//    sei();
//	return 0;			
		
//  while (!(UCSR1A & (1 << UDRE1)));   // wait for Data Register to be empty
//  if (in != out) {                    // if buffer not empty (should always be the case)
//      UDR1 = xmitBuffer[out];
//      out = (out + 1) % XMIT_BUFER_SIZE;
//  }
//  SREG = sreg;
//  sei();
//  return 0;
    /* ****** END DEBUG ****** */
    
    /* If Data Register empty, write next character to USART Data Register. */
    
    if (UCSR1A & (1 << UDRE1)) {        // if Data Register empty
        
        if (in != out) {                // if buffer not empty (should always be the case)
            UDR1 = xmitBuffer[out];
            out = (out + 1) % XMIT_BUFER_SIZE;
        }
    }
    
    /* if buffer not empty, ensure interrupt on Data Register empty */
    
    if (in != out) {
        UCSR1B = UCSR1B | (1 << UDRIE1);    // enable interrupt on Data Register empty  
    }
	
    UCSR1B = UCSR1B | (1 << UDRIE1);    // ****** enable interrupt on Data Register empty  
    
    SREG = sreg;                        // restore previous interrupt state
    sei();                              // FIXME: ******
    return 0;
}



/* uart_output_buffer_empty() - return true if output buffer is empty.
 */
 
volatile int uart_output_buffer_empty() {
	return !(in == out);
}



/* USART Data Register Empty ISR.
 *
 * This function processes USASRT Data Register Empty interrupts.
 * If the buffer is not empty, the next char from the buffer is
 * placed in the Data Register.  USART Data Register Empty interrupts
 * are left enabled (even if this might be the last char in the buffer.
 * This might result in an extra interrupt after the last char 
 * transmitted, but it *might* close a timing window.  I need to look
 * at this more.
 *
 * If the buffer is empty, USART Data Register Empty interrupts are
 * disabled.  
 *
 */
 


 /* USART Data Register Empty ISR handler.
  *
  * This ISR handler handles USART Data Register empty interrupts.
  */

ISR(USART1_UDRE_vect) {

    /* Indicate that ISR(USART_UDRE__vect) caught USART_UDRE interrupt. */

//    onYellowLED();    
    
    /* Move next char to USART Data Register.  If last char, disable nable
     *  USART_UDRE interrupt. */
    
    if (in != out) {                    // if buffer not empty
        UDR1 = xmitBuffer[out];         // put next char in Data Register
        out = (out + 1) % XMIT_BUFER_SIZE;    // update out
    } else {
        UCSR1B = UCSR1B & ~(1 << UDRIE1);    // disable interrupt on Data Register empty
    }
//	offYellowLED();
}



/* waitOUtputComplete - Wait for printing to the console to complete.
 */
 
waitOutputComplete() {
	while (in != out) {                 // wait for output buffer to empty
		_delay_ms(1);
		    if (UCSR1A & (1 << UDRE1)) {        // if Data Register empty
        cli();        
        if (in != out) {                // if buffer not empty (should always be the case)
            UDR1 = xmitBuffer[out];
            out = (out + 1) % XMIT_BUFER_SIZE;
        }
		sei();
    }

        UCSR1B = UCSR1B | (1 << UDRIE1);    // enable interrupt on Data Register empty  
	}
}



/* uart_getchar() - get a character from USART port.
 * 
 * This enables the avr-libc read code to work.  However, this probably
 * interferes with the USART_RX_vect ISR code below, and so use of this
 * function is discouraged.
 */
 
int uart_getchar(FILE *stream) {
    while ( !(UCSR1A & (1<<RXC1)) );
    return UDR1;
}



/* USART Receive Data Ready ISR.
 * This code is copied pretty directly from:
 * https://github.umn.edu/course-material/repo-rtes-public/blob/master/ExampleCode/basic-serial/main.c
 *
 * Modified to ensure command terminated by null character.
 */

ISR(USART1_RX_vect) {

    uint8_t ch = UDR1;                  // fetch character

	if ((recv_buffer_ptr >= RECEIVE_BUFFER_LENGTH-1) && (ch != '\r')) return;    // ignore excess chars
	
	/* Check for command termination. */
	
	if (ch == '\r') {                   // already terminated string
		user_command_ready = 1;
	}

    /* process delete char. */
	
    else if (ch == 8) {
        if (recv_buffer_ptr >= 1)
            --recv_buffer_ptr;
        recv_buffer[recv_buffer_ptr] = 0;
    }

    //Only store alphanumeric symbols, space, the dot, plus and minus sign
    else if
        ( (ch == ' ') || (ch == '.') || (ch == '+') || (ch == '-') || (ch == ':') ||
        ((ch >= '0') && (ch <= '9')) ||
        ((ch >= 'A') && (ch <= 'Z')) ||
        ((ch >= 'a') && (ch <= 'z')) ) {
        recv_buffer[recv_buffer_ptr++] = ch;
		recv_buffer[recv_buffer_ptr] = '\0';
    }
}



	typedef struct {
		char *cmd;
		int (*cmdProc)(char *);
    } commandTable;
	
	commandTable cmdTable[25];
	 
	int commandMax = 0;

	
/* processUserCommand - process user command input.
 */

char* processUserCommand(char* buffer) {
	
	char* token;
	token = strtok(buffer, " ");

    int i;
	int found = 0;
    for (i = 0; i < commandMax; i++) {
	    if (strcmp(token, cmdTable[i].cmd) == 0) {
			return cmdTable[i].cmdProc(buffer);
		    found = 1;					// uhhh... never executed
		    break;						// never executed...
        }			
	}
	
	if (!found) {						// FIXME: no conditional needed
		return "# Command not found\n";
	}
}



/* registerUserCommand - register a user command for command processing. 
 */
 
int registerUserCommand(char* command, char* (*commandProcessor)(char *)) {
	cmdTable[commandMax].cmd = command;
	cmdTable[commandMax++].cmdProc = commandProcessor;
	return 0;
}
