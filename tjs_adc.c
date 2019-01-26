/* tjs_adc.c - ADC functions.
 *
 * Timothy J. Salo, 2018.
 */

#define F_CPU 16000000ul	// required for _delay_ms()

#include <avr/io.h>


/* initAdc - initialize ADC.
 *
 * Initialize ADC with reference voltage of AVcc and prescaler of 128.
 */
 
void initAdc() {
    ADMUX = 0;                          // clear ADC registers
	ADCSRB = 0;
	DIDR0 = 0;
	DIDR1 = 0;
    ADMUX |= (1 << REFS1) | (1 << REFS0);    // reference voltage = 2.56V (for temp sensor)
//    ADMUX |= (1 << REFS0);               // reference voltage = AVcc
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);    // prescaler of 128
    ADCSRA |= (1 << ADEN);              // enable ADC
}



/* readAdc - read ADC value from specified channel.
 */
 
unsigned int readAdc(int channel) {
    ADMUX |= (ADMUX & 0xe0) | (channel & 0x1f);    // lower bits of channel
    if (channel & 0x20) ADCSRB |= (1 << MUX5); else ADCSRB &= ~(1 << MUX5);    // upper bit
    ADCSRA |= (1 << ADSC);      // start ADC conversion
    while (ADCSRA & (1 << ADSC));    // wait for ADC conversion to complete
    return ADC;
}