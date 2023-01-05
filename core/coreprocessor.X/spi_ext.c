/*
 * File:   spi_ext.c
 * Author: greengravity
 *
 * Created on December 31, 2022, 10:15 AM
 */


#include "spi_ext.h"

void SPI1_Transmit16bitRepeated( uint16_t data, int len ) {
//    for (int i=0;i<len;i++) SPI1_Transmit16bit(data);   
    
    int dataSentCount = 0;    
    int dataReceiveCount = 0;
    uint8_t byte1 = (uint8_t)(data>>8);
    uint8_t byte2 = (uint8_t)(data);
    uint8_t p_dummy;
    bool hb = true;
    len = len * 2;    
    // set the pointers and increment delta 
    // for transmit and receive operations
    while( SPI1STATLbits.SPITBF == true ) { }

    while (dataSentCount < len )
    {
        if ( SPI1STATLbits.SPITBF != true )
        {
            
            SPI1BUFL = hb ? byte1 : byte2;
            hb = !hb;
            
            dataSentCount++;
        }
        if ( SPI1STATLbits.SPIRBE != true ) {
            p_dummy = SPI1BUFL;
            dataReceiveCount++;
        }
              
    }
    while (dataReceiveCount < len ) {
        if (SPI1STATLbits.SPIRBE != true) { 
            p_dummy = SPI1BUFL;
            dataReceiveCount++;
        }
    }
            
}

void SPI1_Transmit16bit(uint16_t i) {
    SPI1_Exchange8bit((uint8_t)(i>>8));
    SPI1_Exchange8bit((uint8_t)(i));
}

void SPI1_Transmit32bit(uint32_t i) {
    SPI1_Exchange8bit((uint8_t)(i>>24));    
    SPI1_Exchange8bit((uint8_t)(i>>16));    
    SPI1_Exchange8bit((uint8_t)(i>>8));    
    SPI1_Exchange8bit((uint8_t)i);     

}


void SPI1_transmit16bitBuffer(uint16_t *dataTransmitted, uint16_t wordCount ) {
    for (uint16_t i=0;i<wordCount;i++ ) {
        SPI1_Transmit16bit( dataTransmitted[i] );        
    }    
}