/* tjs_leds.c - miscellaneous LED functions.
 *
 * Copyright (C) Timothy J. Salo, 2018.
 */

#include <avr/io.h>


void enableYellowLED() {                // 
    DDRC |= (1 << DDC7);                // Enable on-board yellow LED for output
}

	
void enableGreenLED() {
    DDRD |= (1 << DDD5);                // Enable on-board green LED for output
}


void enableRedLED() {
    DDRB |= (1 << DDB0);                // Enable on-board red LED for output
}

	
void onYellowLED() {
    DDRC |= (1 << DDC7);                // Enable yellow LED for output
    PORTC |= (1 << PORTC7);             // Turn on on-board yellow LED
 }
 
 
void onGreenLED() {
    DDRD |= (1 << DDD5);                // Enable green LED for output
    PORTD &= ~(1 << PORTD5);            // Turn on on-board green LED
}


void onRedLED() {
    DDRB |= (1 << DDB0);                // Enable red LED for output
    PORTB &= ~(1 << PORTB0);            // turn on on-board red LED
}


void offYellowLED() {
    PORTC &= ~(1 << PORTC7);            // Turn off on-board yellow LED
}


void offGreenLED() {
    PORTD |= (1 << PORTD5);             // Turn off on-board green LED
}


void offRedLED() {
    PORTB |= (1 << PORTB0);             // turn off on-board red LED
}


void toggleYellowLED() {
    PORTC ^= (1 << PORTC7);             // toggle on-board yellow LED
}


void toggleGreenLED() {
    PORTD ^= (1 << PORTD5);             // toggle on-board green LED
}


void toggleRedLED() {
    PORTB ^= (1 << PORTB0);             // toggle on-board red LED
}


void enableExternalYellowLED() {
	DDRD |= (1 << DDD6);                // enable external yellow LED (A-Star pin 12)
}


void enableExternalGreenLED() {
	DDRB |= (1 << DDB6);                // enable external green LED (A-Star pin 10)
}


void enableExternalRedLED() {
	DDRB |= (1 << DDB4);                // enable external red LED (A-Star pin 8)
}


void onExternalYellowLED() {
	DDRD |= (1 << DDD6);                // initialize external yellow LED (A-Star pin 12)
	PORTD &= ~(1 << PD6);
}

void onExternalGreenLED() {
	DDRB |= (1 << DDB6);                // initialize external green LED (A-Star pin 10)
	PORTB &= ~(1 << PB6);
}


void onExternalRedLED() {
	DDRB |= (1 << DDB4);                // initialize external red LED (A-Star pin 8)
    PORTB &= ~(1 << PB4);
}


void offExternalYellowLED() {
	PORTD &= ~(1 << PD6);               // FIXME: wrong
}	


void offExternalGreenLED() {
    PORTB &= ~(1 << PB6);               // FIXME: wrong
}


void offExternalRedLED() {
	PORTB &= ~(1 << PB4);               // FIXME: wrong
}


void toggleExternalYellowLED() {
    PORTD ^= (1 << PD6);
}


void toggleExternalGreenLED() {
    PORTB ^= (1 << PB6);
}


void toggleExternalRedLED(){
	PORTB ^= (1 << PB4);
}



void displayOctalDigit(int n) {
	enableYellowLED();
	enableGreenLED();
	enableRedLED();
	
	if (n & 0x4) 
		onYellowLED();
	else
		offYellowLED();
	
	if (n & 0x2)
		onGreenLED();
	else
		offGreenLED();
	
	if (n & 0x1) 
		onRedLED();
	else
		offRedLED();
}