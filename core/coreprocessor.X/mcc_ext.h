/* Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */

/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef XC_MCC_EXT_H
#define	XC_MCC_EXT_H

#include <stdbool.h>
#include "mcc_generated_files/fatfs/ff.h"

// ********** Standard functions *******
void bootPeripherals();
void shutdownPeripherals();
void setSleep();

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
bool isKeySet( );
void setMasterKey(uint8_t *key);
void swipeKeys();
bool verifyMasterKey();


void prepareAES128BitCBC();
bool prepare128bitEncryption( uint8_t *iv );
bool prepare128bitDecryption( uint8_t *iv );
void encrypt128bit( uint8_t *plaintext, uint8_t* ciphertext );
void decrypt128bit( uint8_t* ciphertext, uint8_t *plaintext );
void endEncryption( );
void generateRND(uint8_t *rnd);

#endif	/* XC_MCC_EXT_H */

