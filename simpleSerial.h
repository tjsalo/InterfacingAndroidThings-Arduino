/* simpleSerial.h - simple I/O using avr-libc.
 *
 * Largely copied from comments in AVR stdio.h
 *
 * Highly recommend reading the comments in stdio.h, if you want to understand
 * how this works.
 *
 * Timothy J. Salo, Setember 2018.
 */

//#define RECEIVE_BUFFER_LENGTH 50

int uart_putchar(char c, FILE *stream); // write a character to USART
int uart_getchar(FILE *stream);         // Get a character from USART

void uart_init(void);                   // Initial USART

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
static FILE mystdin = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

int uart_output_buffer_empty();         // check if output buffer is empty
void waitOutputComplete();              // wait for output to finish

char* processUserCommand(char*);   		// process command from interface
int registerUserCommand(char*, char* (*cmdProc)(char *));    // register a user command
