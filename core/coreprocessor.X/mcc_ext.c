/*
 * File:   mcc_ext.c
 * Author: greengravity
 *
 * Created on January 17, 2023, 11:08 PM
 */


#include "xc.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "mcc_ext.h"
#include "mcc_generated_files/system.h" 
#include "mcc_generated_files/spi1_driver.h"
#include "mcc_generated_files/fatfs/ff.h"
#include "mcc_generated_files/pin_manager.h"


// ********* Extend OC1 Functions *********
uint16_t OC1_PrimaryValueGet() {
    return OC1R;   
}

// ********* Extend Filesystem Functions *********

typedef struct { uint16_t con1; uint16_t brg; uint8_t operation;} spi1_configuration;

static uint8_t _cspin; 
static spi1_configuration _spi1_config;
static bool _spi1_spien;


void fs_standby() {
    _cspin = SDCard_CS_GetValue();
    SDCard_CS_SetHigh();
    if ( SPI1CON1Lbits.SPIEN == 1 ) {
        _spi1_spien = true;        
        _spi1_config.con1 = SPI1CON1L;
        _spi1_config.brg = SPI1BRGL;
        _spi1_config.operation = TRISBbits.TRISB9;      
        SPI1CON1Lbits.SPIEN = 0;
    } else {
        _spi1_spien = false;
    } 
}

void fs_standby_file(FIL* fp) {
    f_sync(fp);
    fs_standby();    
}

void fs_resume() {
    
    if ( _spi1_spien ) {
        SPI1CON1L = _spi1_config.con1;
        SPI1BRGL = _spi1_config.brg;
        TRISBbits.TRISB9 = _spi1_config.operation;
        SPI1CON1Lbits.SPIEN;
    }
    
    if ( _cspin ) {
        SDCard_CS_SetHigh();
    } else {
        SDCard_CS_SetLow();
    }
}




// ************* Extending SPI-Functions ********
void SPI1_Transmit16bitRepeated( uint16_t data, int len ) {

    int dataSentCount = 0;    
    uint8_t byte1 = (uint8_t)(data>>8);
    uint8_t byte2 = (uint8_t)(data);
    uint8_t p_dummy;

    while (dataSentCount < len )
    {
        while ( SPI1STATLbits.SPIRBF ) { }        
        SPI1BUFL = byte1;
        while ( SPI1STATLbits.SPIRBE ) { }                    
        p_dummy = SPI1BUFL;
               
        while ( SPI1STATLbits.SPIRBF ) { }
        SPI1BUFL = byte2;                
        while ( SPI1STATLbits.SPIRBE ) { }
        p_dummy = SPI1BUFL;

        dataSentCount++;        
    }
}


void SPI1_Transmit16bit(uint16_t data) {   
    uint8_t byte1 = (uint8_t)(data>>8);
    uint8_t byte2 = (uint8_t)(data);
    uint8_t p_dummy;

    while ( SPI1STATLbits.SPIRBF ) { }        
    SPI1BUFL = byte1;
    while ( SPI1STATLbits.SPIRBE ) { }                    
    p_dummy = SPI1BUFL;

    while ( SPI1STATLbits.SPIRBF ) { }
    SPI1BUFL = byte2;                
    while ( SPI1STATLbits.SPIRBE ) { }
    p_dummy = SPI1BUFL;
}

void SPI1_Transmit32bit(uint32_t data) {
    
    uint8_t p_dummy;

    uint8_t byte1 = (uint8_t)(data>>24);
    uint8_t byte2 = (uint8_t)(data>>16);
    uint8_t byte3 = (uint8_t)(data>>8);
    uint8_t byte4 = (uint8_t)(data);
    

    while ( SPI1STATLbits.SPIRBF ) { }        
    SPI1BUFL = byte1;
    while ( SPI1STATLbits.SPIRBE ) { }                    
    p_dummy = SPI1BUFL;

    while ( SPI1STATLbits.SPIRBF ) { }
    SPI1BUFL = byte2;                
    while ( SPI1STATLbits.SPIRBE ) { }
    p_dummy = SPI1BUFL;    
    
    while ( SPI1STATLbits.SPIRBF ) { }
    SPI1BUFL = byte3;                
    while ( SPI1STATLbits.SPIRBE ) { }
    p_dummy = SPI1BUFL;   

    while ( SPI1STATLbits.SPIRBF ) { }
    SPI1BUFL = byte4;                
    while ( SPI1STATLbits.SPIRBE ) { }
    p_dummy = SPI1BUFL;   
    
}


void SPI1_transmit16bitBuffer(uint16_t *dataTransmitted, uint16_t wordCount ) {
    for (uint16_t i=0;i<wordCount;i++ ) {
        SPI1_Transmit16bit( dataTransmitted[i] );        
    }    
}
