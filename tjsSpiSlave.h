/* tjsSpiSlave.h - Implement SPI slave.
 *
 */

#ifndef TJS_SPI_SLAVE_H
#define TJS_SPI_SLAVE_H

#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#define SPI_RX_BUFFER_LENGTH 50
#define SPI_TX_BUFFER_LENGTH 50

#define SPI_PORT PORTB
#define SPI_DDR  DDRB


void tjsSpiInit();
void tjsSpiSendBytes(void);

void tjsSpiStop(void);

void tjsSpiReq();

#endif
