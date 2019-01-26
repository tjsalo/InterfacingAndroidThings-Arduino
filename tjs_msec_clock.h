/* tjs_msec_clock.h - implement millisecond clock.
 *
 * The millisecond clock is incremented every millisecond by timer 0.
 *
 * Copyright (C) Timothy J. Salo, 2018.
 */

void initializeMsecClock(void);           // initialize millisecond clock
void stopMsecClock(void);                 // stop millisecond clock
void startMsecClock(void);                // stop millisecond clock
unsigned long int getMsecClock(void);     // get current millisecond clock
