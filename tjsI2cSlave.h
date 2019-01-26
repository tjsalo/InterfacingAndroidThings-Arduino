/* tjs_i2cSlave.h - Implement I2C slave.
 *
 * Loosely based on “AVR I2C Slave Code” by GitHub user thegouger.
 * <https://github.com/thegouger/avr-i2c-slave>
 */

#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#define I2C_RX_BUFFER_LENGTH 50
#define I2C_TX_BUFFER_LENGTH 50

void tjsI2cInit(uint8_t address);
void tjsI2cSendBytes(void);

void I2C_stop(void);

//void I2C_recv(uint8_t);
void I2C_req();


#endif
