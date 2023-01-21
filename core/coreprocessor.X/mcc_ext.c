/*
 * File:   mcc_ext.c
 * Author: greengravity
 *
 * Created on January 17, 2023, 11:08 PM
 */

#include "main.h"
#include "xc.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "mcc_ext.h"
#include "mcc_generated_files/system.h" 
#include "mcc_generated_files/spi1_driver.h"
#include "mcc_generated_files/fatfs/ff.h"
#include "mcc_generated_files/pin_manager.h"
#include "mcc_generated_files/ext_int.h"
#include "mcc_generated_files/adc1.h"

#include "display_driver.h"
#include "buttons.h"

// ********** Standard functions *******
void bootPeripherals() {
    EX_INT0_InterruptDisable();
    PER_PWR_SetLow();
    VOLT_PWR_SetLow();
    __delay_ms(150);

    spi1_open(DISPLAY_CONFIG);
    dispStart();
    spi1_close();
  
    
    ADC1_ChannelSelect(VOLT_READ);
    ADC1_Enable();
    ADC1_SoftwareTriggerDisable();

    
    updateButtons(true);
    __delay_ms(50);
    updateButtons(true);
}

void shutdownPeripherals() {   
    dispStop();
    spi1_close();
    SDCard_CS_SetHigh();
    PER_PWR_SetHigh(); //Unpower Peripherie
    VOLT_PWR_SetHigh();   //Unpower Voltagedivider
    //ADC1_Disable();    //Stop Analogconverter    
}


void setSleep() {
    //Shutdown all Peripherals
    shutdownPeripherals();
    _LATB2 = 1;
    EX_INT0_InterruptFlagClear();
    EX_INT0_InterruptEnable();
    Sleep();
    Nop();            
    EX_INT0_InterruptDisable();
    EX_INT0_InterruptFlagClear();
    __delay_ms(100);
    
    bootPeripherals();
}



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



// AES-Encryption and Decryption functions
static uint8_t masterkey[16];
static bool keyset=false;

//128bit AES key
bool isKeySet( ) {
    return keyset;
}

void setMasterKey(uint8_t *key) {
    keyset = true;
    memcpy( masterkey, key, 16 );
}

void swipeKeys() {    
    keyset = false;
    memset( masterkey, 0x00, 16);
    memcpy( (void*)&CRYKEY0, masterkey, 16); 
}

bool verifyMasterKey() {
    return true;    
}


void prepareAES128BitCBC() {
    CRYCONLbits.CRYON = 1;
    CRYCONHbits.KEYSRC = 0b0000; //RAM crykey  
    CRYCONLbits.CPHRSEL = 1;     //AES-engine
    CRYCONHbits.KEYMOD = 0;      //128 bit key
    CRYCONLbits.CPHRMOD = 1;
}


//prepare for 128bit decryption by setting iv and masterkey with current setup
bool prepare128bitEncryption( uint8_t *iv ) {
    if ( !keyset ) return false;
    
    CRYCONLbits.CPHRMOD = 1;
    CRYCONLbits.OPMOD = 0b0000;  //Operation mode encryption
    
    memcpy( (void*)&CRYKEY0, masterkey, 16);   //Set Masterkey
    memcpy( (void*)&CRYTXTB, iv, 16 );        //Set initialization Vector     
    
    return true;
}


//prepare for 128bit decryption by setting iv and masterkey with current setup
bool prepare128bitDecryption( uint8_t *iv ) {
    if ( !keyset ) return false;
    
    //Generate decryption key via AES-Keyexpand
    CRYCONLbits.OPMOD = 0b0010;  //Key Expand mode    
    memcpy( (void*)&CRYKEY0, masterkey, 16);   //Set Masterkey
    CRYCONLbits.CRYGO = 0b1;    //Start keygeneration
    while (CRYCONLbits.CRYGO == 0b1 );
    
    CRYCONLbits.OPMOD = 0b0001;  //Operation mode decryption        
    memcpy( (void*)&CRYTXTA, iv, 16);   //Set initialization Vector     
    
    return true;
}


//encrypt a block of 128 bit with the current setup
void encrypt128bit( uint8_t *plaintext, uint8_t* ciphertext ) {
    memcpy((void*)&CRYTXTA, plaintext, 16);
    CRYCONLbits.CRYGO = 1;
    while ( CRYCONLbits.CRYGO == 1 );    
    memcpy(ciphertext, (void*)&CRYTXTB, 16);
}

//decrypt a block of 128 bit with the current setup
void decrypt128bit( uint8_t* ciphertext, uint8_t *plaintext ) {
    memcpy((void*)&CRYTXTB, ciphertext, 16);
    CRYCONLbits.CRYGO = 1;
    while ( CRYCONLbits.CRYGO == 1 );    
    memcpy(plaintext, (void*)&CRYTXTC, 16);
}

//clear keyram and stop engine
void endEncryption( ) {        
    for ( int i= 0;i<16;i++) ((uint8_t*)&CRYKEY0)[i] = 0x00;
    CRYCONLbits.CRYON = 0;
}

//Generate 16bit random bytes
void generateRND(uint8_t *rnd) { 
    uint8_t con = CRYCONLbits.CRYON;
    
    CRYCONLbits.CRYON = 0b1; /* Turn module on */
    CRYCONLbits.OPMOD = 0b1010; /* Select to generate a random number */
    CRYCONLbits.CRYGO = 1; /* Start the process */
    while(CRYCONLbits.CRYGO == 1){} /* Wait until the module is done */
    memcpy(rnd, (void*)&CRYTXTA, 16);
    
    /* The random number is now located in CRYTXTA. */    
    
    CRYCONLbits.CRYON = con;
}