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
#include "mcc_generated_files/fatfs/ff.h"
#include "logic.h"

// ********** Standard functions *******
void bootPeripherals(APP_CONTEXT *ctx);
void shutdownPeripherals(APP_CONTEXT *ctx);
void setSleep(APP_CONTEXT *ctx);

// ********* Extend OC1 Functions *********
uint16_t OC1_PrimaryValueGet();

// ********* Extend Filesystem Functions *********
void fs_standby();
void fs_standby_file(FIL* fp);
void fs_resume();

// ********* Extend SPI-Functions *********
void SPI1_Transmit16bitRepeated( uint16_t data, int len );
void SPI1_Transmit16bit(uint16_t data);
void SPI1_Transmit32bit(uint32_t data);
void SPI1_transmit16bitBuffer(uint16_t *data, uint16_t wordCount );


// AES-Encryption and Decryption functions
void prepareAES128BitCBC();
bool prepare128bitEncryption( uint8_t *iv );
bool prepare128bitDecryption( uint8_t *iv );
void encrypt128bit( uint8_t *plaintext, uint8_t* ciphertext );
void decrypt128bit( uint8_t* ciphertext, uint8_t *plaintext );
void endEncryption( );
void generateRND(uint8_t *rnd);

#endif	/* XC_MCC_EXT_H */


