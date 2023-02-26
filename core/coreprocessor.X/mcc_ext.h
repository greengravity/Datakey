/* 
 * File:   
 * Author: greengravity
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef XC_MCC_EXT_H
#define	XC_MCC_EXT_H

#include <stdbool.h>
#include "fs/ff.h"
#include "logic.h"

#define OC1_TOPVALUE 0x3F80 

#define SDCard_CD_SetWPUOn()           (CNPU2bits.CN25PUE = 1)
#define SDCard_CD_SetWPUOff()          (CNPU2bits.CN25PUE = 0)

#define MISO_SetWPUOn()           (CNPU2bits.CN18PUE = 1)
#define MISO_SetWPUOff()          (CNPU2bits.CN18PUE = 0)

#define MISO_SetWPDOn()           (CNPD2bits.CN18PDE = 1)
#define MISO_SetWPDOff()          (CNPD2bits.CN18PDE = 0)


#define SHUTDOWN_POWER_LIMIT 3

extern volatile UINT buttonTimer, adcTimer, usbTimeout, keyTimeout, busChargeStartTimer, gameTimer;


// ********** Standard functions *******
void disableVoltPower();
void enableVoltPower();
uint8_t calculateBatteryPowerFromADC( int adcin );

void enableBUSCharge();
void disableBUSCharge();
bool isBUSChargeEnabled();

bool bootPeripherals(APP_CONTEXT *ctx);
void shutdownPeripherals(APP_CONTEXT *ctx);
void setSleep(APP_CONTEXT *ctx);

// ********* Extend OC1 Functions *********
uint16_t OC1_PrimaryValueGet();

// ********* Extend Filesystem Functions *********
void mountFS( APP_CONTEXT *ctx );
void unmountFS( APP_CONTEXT *ctx );

// ********* Extend SPI-Functions *********
void SPI1_Transmit16bitRepeated( uint16_t data, uint16_t len );
void SPI1_Transmit16bit(uint16_t data);
void SPI1_Transmit32bit(uint32_t data);
void SPI1_transmit16bitBuffer(uint16_t *data, uint16_t wordCount );


// AES-Encryption and Decryption functions
void prepareAES128BitCBC();
bool prepare128bitEncryption( uint8_t *iv );
bool prepare128bitDecryption( uint8_t *iv );
void switch128BitDecryptEncrypt();
void encrypt128bit( uint8_t *plaintext, uint8_t* ciphertext );
void decrypt128bit( uint8_t* ciphertext, uint8_t *plaintext );
void endEncryption( );
void generateRND(uint8_t *rnd);

#endif	/* XC_MCC_EXT_H */


