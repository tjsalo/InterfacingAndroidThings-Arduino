/* tjs_msec_clock.c - implement millisecond clock.
 *
 * The msec clock is incremented every msec by timer 4.
 *
 * Copyright (C) Timothy J. Salo, 2018.
 */

 
#ifndef F_CPU
#define F_CPU 16000000UL
#endif


#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "tjs_leds.h"


/* 64-bit millisecond clock. */

volatile static unsigned long long int msec_clock = 0;     // 64-bit millisecond clock


/* initializeMsecClock - initialize millisecond clock.
 *
 * initializeCpuClock sets up timer 4 to drive the millisecond clock.
 * Note: initializeMsecClock() does _not_ start timer4.
 */
 
void initializeMsecClock(void) {
	
	TCCR4A = TCCR4B = TCCR4C = TCCR4D = TCCR4E = 0;  // clear timer 4 control registers
	TCNT4 = TC4H = OCR4A = OCR4B = OCR4C = OCR4D = 0;
	TIMSK4 = TIFR4 = DT4 = 0;           // FIXEME: be judicious in clearing registers
	
    TCCR4B |= (1 << CS42) | (1 << CS41) | (1 << CS40);  // set prescaler of 64
	
//	OCR4A = 250;                        // compare value
//	OCR4B = 250;
	OCR4C = 250;
//	OCR4D = 250;
	
    TIMSK4 |= (1 << OCIE4A);
//    TIMSK4 |= (1 << OCIE4D) | (1 << OCIE4A) | (1 << OCIE4B) | (1 << OCIE4B);
//	TIMSK4 |= (1 << TOV4);
//    TIFR4 |= (1 << OCIE4D) | (1 << OCIE4A) | (1 << OCIE4B) | (1 << OCIE4B);
//	TIFR4 |= (1 << TOV4);
	
//    TCCR0A = TCCR0B = 0;                // clear timer0 control registers
//    TCCR0A |= (1 << WGM01);             // set Clear Timer on Compare Match (CTC) mode 
//	TCCR0B |= (1 << CS01) | (1 << CS00);    // set prescaler of 64
//    OCR0A = 250;

	msec_clock = 0;                     // clear millisecond clock
}



/* stopMsecClock - stop millisecond clock.
 */
 
void stopMsecClock() {
	TIFR4 &= ~(1 << OCIE4D) & ~(1 << OCIE4A) & ~(1 << OCIE4B) & ~(1 << OCIE4B);
	TIFR4 &= ~(1 << TOV4);

//    TIMSK0 &= ~(1<<OCIE0A);              // disable timer0 interrupts
}


/* startMillisecondClock() - start millisecond clock.
 */
 
void startMsecClock() {
    TIMSK4 |= (1 << OCIE4A);
//    TIMSK4 |= (1 << OCIE4D) | (1 << OCIE4A) | (1 << OCIE4B) | (1 << OCIE4B);
//	TIMSK4 |= (1 << TOV4);
//    TIFR4 |= (1 << OCIE4D) | (1 << OCIE4A) | (1 << OCIE4B) | (1 << OCIE4B);
//	TIFR4 |= (1 << TOV4);
//    TIMSK0 |= (1<<OCIE0A);              // enable interrupts
}





/* getMsecClock() - get current millisecond clock value.
 *
 * Note: users should probably check whether a timer overflow interrupt
 * is pending, and adjust the values of cpu_clock_upper, as necessary.
 */

unsigned long int getMsecClock() {

    unsigned char sreg;                 // save status register (interrupt state)
    unsigned long int temp;

    sreg = SREG;	
    cli();
    temp = msec_clock;
    SREG=sreg;
    return temp;
}


//ISR(TIMER0_COMPA_vect) {
//    msec_clock++;
//}
	

/* Timer4 Compare Match A ISR.
 */
 
ISR(TIMER4_COMPA_vect) {
	msec_clock++;                       // increment msec clock
}
	 

/* Timer4 Compare Match B ISR.
 */
 
ISR(TIMER4_COMPB_vect) {
//	onYellowLED();
//	msec_clock++;                       // increment msec clock
}


/* Timer4 Compare Match C ISR.
 */
 
ISR(TIMER4_COMPC_vect) {
//	onYellowLED();
//	msec_clock++;                       // increment msec clock
}


/* Timer4 Compare Match D ISR.
 */
 
ISR(TIMER4_COMPD_vect) {
//	onYellowLED();
//	msec_clock++;                       // increment msec clock
}

	 
/* Timer4 Timer Overflow ISR.
 */
 
ISR(TIMER4_OVF_vect) {
//	onYellowLED();
//	msec_clock++;                       // increment msec clock
}