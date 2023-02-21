/*
 * File:   mcc_ext.c
 * Author: greengravity
 *
 * Created on January 17, 2023, 11:08 PM
 */

#include "main.h"
#include <xc.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "mcc_ext.h"
#include "mcc_generated_files/system.h" 
#include "mcc_generated_files/spi1_driver.h"
#include "mcc_generated_files/pin_manager.h"
#include "mcc_generated_files/ext_int.h"
//#include "mcc_generated_files/adc1.h"
#include "mcc_generated_files/tmr2.h"
#include "mcc_generated_files/oc1.h"

#include "fs/ff.h"
#include "fs/diskio.h"
#include "logic.h"
#include "display_driver.h"
#include "buttons.h"

#include "usb/usb.h"


volatile UINT buttonTimer = 0, adcTimer = 0, usbTimeout = 0, keyTimeout = 0, busChargeStartTimer = 0;


// ********** Standard functions *******
void disableVoltPower() {
    VOLT_PWR_SetDigitalInput();       
}

void enableVoltPower() {
    VOLT_PWR_SetDigitalOutput();
    VOLT_PWR_SetLow();
}

void enableBUSCharge() {
    CHRG_PWR_SetDigitalOutput();
    CHRG_PWR_SetLow();    
}

void disableBUSCharge() {    
    CHRG_PWR_SetDigitalInput();          
}

bool isBUSChargeEnabled() {
    return _TRISC3 == 0;
}


void mountFS( APP_CONTEXT *ctx ) {   
    if (f_mount(&ctx->drive,"0:",1) == FR_OK) {
        ctx->fsmounted = true;
        return;
    }    
    ctx->fsmounted = false;
}

void unmountFS( APP_CONTEXT *ctx ) {
    ctx->fsmounted = false;
    f_mount(0,"0:",0);
}

uint8_t calculateBatteryPowerFromADC( int adcin ) {
    //return adcin >> 3; 
    double v = adcin;
    uint8_t vr;
    v /= 420; //Divider to get from analoginput to voltage
    v -= 3.15; //3.15V is the bottom limit result in 0%
    if ( v < 0 ) v = 0;
    v = ( v / ( 3.8 - 3.15 ) ) * 100; //3.9V is the top charge limit result in 100%
    if ( v > 100 ) v = 100;
    
    vr = (uint8_t)v;
    if ( vr <= SHUTDOWN_POWER_LIMIT ) vr = SHUTDOWN_POWER_LIMIT + 1; 
    return vr;
}


bool bootPeripherals(APP_CONTEXT *ctx) {
    EX_INT0_InterruptDisable();
    PER_PWR_SetLow();    
    enableVoltPower();

    SDCard_CD_SetWPUOn();
    SDCard_CS_SetHigh();
       
    CS_D_SetHigh();
    RS_D_SetHigh();

    // Configure the ADC module
    AD1CON1 = 0x8474; // Turn off the ADC module
    AD1CON2 = 0; // Select AVdd and AVss as reference voltages
    //AD1CON3 = 0; // Use default sample time and conversion clock
    AD1CON3 = 0x1003;
    AD1CHS = 0x6; // Select RB14 as the input channel
    AD1CON1bits.ADON = 1; // Turn on the ADC module      
             
    __delay_ms(150);

    TMR2_Start();

    disableBUSCharge(ctx);
    USBDeviceInit();
        
    updateButtons(true);
    updateButtons(true);    
    
    
    ctx->adc_rw_state = 0;
    if ( !USB_BUS_SENSE ) {
        //Check if voltage is sufficient to start        
        AD1CON1bits.SAMP = 1;
        while ( !AD1CON1bits.DONE ) { }

        int analogValue = ADC1BUF0;
        ctx->power = calculateBatteryPowerFromADC( analogValue );
        
        disableVoltPower();
        
        if ( ctx->power <= SHUTDOWN_POWER_LIMIT ) {
            return false;
        }
    } else {
       ctx->power = 0xff;
       disableVoltPower();
    }
        
    spi1_open(DISPLAY_CONFIG);
    dispStart();
    spi1_close();
                     
    /*
    AD1PCFGbits.PCFG14 = 0;
    ADC1_ChannelSelect(VOLT_READ);
    ADC1_Enable();
    adcTimer = 0;
    ctx->adc_rw_state = 1;
    */
        

    
    return true;
}

void shutdownPeripherals(APP_CONTEXT *ctx) {   
    spi_fat_open();    
    unmountFS(ctx);    
    spi_fat_close();
    
    SDCard_CS_SetLow();
    SDCard_CD_SetWPUOff();
    CS_D_SetLow();
    RS_D_SetLow();
    RES_D_SetLow();
    PER_PWR_SetHigh(); //Unpower Peripherie    
    
    OC1_Stop();
    _LATB2=1; //OC1 Pin set to high 
        
    TMR2_Stop();
    CRYCONLbits.CRYON = 0;
        
    //disableVoltPower();
    //ADC1_Disable();    //Stop Analogconverter    
    AD1CON1bits.SAMP = 0;
    AD1CON1bits.ADON = 0; // Turn off the ADC module 
           
    USBDeviceDetach();
    USBModuleDisable();
}


void setSleep(APP_CONTEXT *ctx) {
    //Shutdown all Peripherals            
    
        
    if ( !isPinSet() ) {
        swipeKeys();
    }
    
    
    while ( true ) {
        shutdownPeripherals(ctx);
        setInitialContext( ctx );
        _LATB2 = 1;
        EX_INT0_InterruptFlagClear();
        EX_INT0_InterruptEnable();
        Sleep();
        Nop();
        EX_INT0_InterruptDisable();
        EX_INT0_InterruptFlagClear();
        __delay_ms(100); 
        setInitialContext( ctx );
        if ( bootPeripherals(ctx) ) break;
    }
    
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
static bool fs_isstandby = false;

void fs_standby() {
    if ( !fs_isstandby ) {
        fs_isstandby = true;
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
}

void fs_standby_file(FIL* fp) {
    if ( !fs_isstandby ) {
        f_sync(fp);
        fs_standby();
    }
}

void fs_resume() {
    if ( fs_isstandby ) {
         fs_isstandby = false;
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
}




// ************* Extending SPI-Functions ********
void SPI1_Transmit16bitRepeated( uint16_t data, uint16_t len ) {

    uint16_t dataSentCount = 0;    
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
void prepareAES128BitCBC() {
    CRYCONLbits.CRYON = 1;
    CRYCONHbits.KEYSRC = 0b0000; //RAM crykey  
    CRYCONLbits.CPHRSEL = 1;     //AES-engine
    CRYCONHbits.KEYMOD = 0;      //128 bit key
    CRYCONLbits.CPHRMOD = 1;
}


//prepare for 128bit decryption by setting iv and masterkey with current setup
bool prepare128bitEncryption( uint8_t *iv ) {
    if ( !isKeySet() ) return false;
    
    CRYCONLbits.OPMOD = 0b0000;  //Operation mode encryption
    
    memcpy( (void*)&CRYKEY0, getMasterKey(), 16);   //Set Masterkey
    memcpy( (void*)&CRYTXTB, iv, 16 );        //Set initialization Vector     
    
    return true;
}


//prepare for 128bit decryption by setting iv and masterkey with current setup
bool prepare128bitDecryption( uint8_t *iv ) {
    if ( !isKeySet() ) return false;
    
    //Generate decryption key via AES-Keyexpand
    CRYCONLbits.OPMOD = 0b0010;  //Key Expand mode    
    memcpy( (void*)&CRYKEY0, getMasterKey(), 16);   //Set Masterkey
    CRYCONLbits.CRYGO = 0b1;    //Start keygeneration
    while (CRYCONLbits.CRYGO == 0b1 );
    
    CRYCONLbits.OPMOD = 0b0001;  //Operation mode decryption        
    memcpy( (void*)&CRYTXTA, iv, 16);   //Set initialization Vector     
    
    return true;
}

uint8_t switchregister[48];
bool switchregset= false;

void switch128BitDecryptEncrypt() {
    
    uint8_t swreg[48];
    memcpy( swreg, switchregister, 48); //backup siwtchregister
    memcpy( switchregister, (void*)&CRYTXTA, 16); //copy crytextregister A-C into switchregister
    memcpy( switchregister+16, (void*)&CRYTXTB, 16);            
    memcpy( switchregister+32, (void*)&CRYTXTC, 16);            
    switchregset= true;       
    
    if ( CRYCONLbits.OPMOD == 0b0000 ) {
        //current mode encryption, switch to dencryption
        
        CRYCONLbits.OPMOD = 0b0010;  //Key Expand mode    
        memcpy( (void*)&CRYKEY0, getMasterKey(), 16);   //Set Masterkey
        CRYCONLbits.CRYGO = 0b1;    //Start keygeneration
        while (CRYCONLbits.CRYGO == 0b1 );
        CRYCONLbits.OPMOD = 0b0001;
        
        memcpy((void*)&CRYTXTA, swreg, 16 );
        memcpy((void*)&CRYTXTB, swreg+16, 16 );
        memcpy((void*)&CRYTXTC, swreg+32, 16 );
        
    } else if ( CRYCONLbits.OPMOD == 0b0001 ) {
        //current mode decryption, switch to encryption

        CRYCONLbits.OPMOD = 0b0000;  //Operation mode encryption

        memcpy( (void*)&CRYKEY0, getMasterKey(), 16);   //Set Masterkey
        memcpy((void*)&CRYTXTA, swreg, 16 );
        memcpy((void*)&CRYTXTB, swreg+16, 16 );
        memcpy((void*)&CRYTXTC, swreg+32, 16 );
    }
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
    for ( int i= 0;i<16;i++) {
        ((uint8_t*)&CRYKEY0)[i] = 0x00;
        ((uint8_t*)&CRYTXTA)[i] = 0x00;
        ((uint8_t*)&CRYTXTB)[i] = 0x00;
        ((uint8_t*)&CRYTXTC)[i] = 0x00;
    }
    CRYCONLbits.CRYON = 0;
    if ( switchregset ) {
        switchregset = false;
        memset(switchregister, 0x00, 48);
    }
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



void TMR2_CallBack(void) {
    // 2ms Timer callback    
	UINT n;

    FatTimerCallback();

	n = buttonTimer;					// 1000Hz decrement timer with zero stopped 
	if (n) buttonTimer = --n;
	n = adcTimer;
	if (n) adcTimer = --n;
	n = usbTimeout;					// 1000Hz decrement timer with zero stopped 
	if (n) usbTimeout = --n;   
	n = keyTimeout;					// 1000Hz decrement timer with zero stopped 
	if (n) keyTimeout = --n;        
  	n = busChargeStartTimer;		    // 1000Hz decrement timer with zero stopped 
	if (n) busChargeStartTimer = --n;        
}