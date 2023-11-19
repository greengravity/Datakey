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
#include "usb/usb_tasks.h"
//Various timer for different tasks, controlled by TMR2
volatile UINT buttonTimer = 0, adcTimer = 0, usbTimeout = 0, keyTimeout = 0, busChargeStartTimer = 0, gameTimer = 0;


// ********** Standard functions *******

void disableVoltPower() {
    VOLT_PWR_SetDigitalInput();
}

void enableVoltPower() {
    //enable voltage power divider for measurement with adc
    VOLT_PWR_SetLow();
    VOLT_PWR_SetDigitalOutput();
}

void disablePullups() {
    //Set all buttons and sdcard-detection to low output
    //for some reason the intern pullups consume a low of power in sleep mode
    BTN1_SetLow();
    BTN1_SetDigitalOutput();

    BTN2_SetLow();
    BTN2_SetDigitalOutput();

    BTN3_SetLow();
    BTN3_SetDigitalOutput();

    BTN4_SetLow();
    BTN4_SetDigitalOutput();

    BTN5_SetLow();
    BTN5_SetDigitalOutput();

    BTN6_SetLow();
    BTN6_SetDigitalOutput();

    SDCard_CD_SetLow();
    SDCard_CD_SetDigitalOutput();

    //Disable all pullups except the case sensor
    CNPU1 = 0x0000;
    CNPU2 = 0x0080;
    CNPU3 = 0x0000;

}

void enablePullups() {
    //Set all buttons and sdcard-detection to low output     
    BTN1_SetDigitalInput();
    BTN2_SetDigitalInput();
    BTN3_SetDigitalInput();
    BTN4_SetDigitalInput();
    BTN5_SetDigitalInput();
    BTN6_SetDigitalInput();
    SDCard_CD_SetDigitalInput();

    //Enable all pullups again
    CNPU1 = 0x0700;
    CNPU2 = 0x4280;
    CNPU3 = 0x000C;

}

void enableBUSCharge() {
    CHRG_PWR_SetLow();
    CHRG_PWR_SetDigitalOutput();
}

void disableBUSCharge() {
    CHRG_PWR_SetDigitalInput();
}

bool isBUSChargeEnabled() {
    return _TRISC3 == 0;
}

void mountFS(APP_CONTEXT *ctx) {
    if (!ctx->fsmounted) {
        if (f_mount(&ctx->drive, "0:", 1) == FR_OK) {
            ctx->fsmounted = true;
            return;
        }
        ctx->fsmounted = false;
    }
}

void unmountFS(APP_CONTEXT *ctx) {
    if (ctx->fsmounted) {
        ctx->fsmounted = false;
        f_mount(0, "0:", 0);
    }
}

uint8_t calculateBatteryPowerFromADC(int adcin) {
    //return adcin >> 3; 
    double v = adcin;
    uint8_t vr;
    v /= 420; //Divider to get from analoginput to voltage
    v -= 3.15; //3.15V is the bottom limit result in 0%
    if (v < 0) v = 0;
    v = (v / (3.8 - 3.15)) * 100; //3.9V is the top charge limit result in 100%
    if (v > 100) v = 100;

    vr = (uint8_t) v;
    if (vr <= SHUTDOWN_POWER_LIMIT) vr = SHUTDOWN_POWER_LIMIT + 1;
    return vr;
}

bool bootPeripherals(APP_CONTEXT *ctx, bool wakeFromSleep) {

    enablePullups();

    MISO_SetWPDOff();
    MISO_SetWPUOn();
    __delay_ms(100);

    SDCard_CS_SetHigh();
    CS_D_SetHigh();
    RS_D_SetHigh();
    PER_PWR_SetLow();
    enableVoltPower();

    // Configure the ADC module
    AD1CON1bits.ADON = 1; // Turn on the ADC module      

    __delay_ms(150);

    //check if battery voltage is sufficient, otherwise abort the 
    //boot sequence and tell the sleep method to stay in sleep mode
    ctx->adc_rw_state = 0;
    if (!USB_BUS_SENSE) {
        //Check if voltage is sufficient to start        
        AD1CON1bits.SAMP = 1;
        while (!AD1CON1bits.DONE) {
        }

        int analogValue = ADC1BUF0;
        ctx->power = calculateBatteryPowerFromADC(analogValue);

        disableVoltPower();

        if (ctx->power <= SHUTDOWN_POWER_LIMIT) {
            return false;
        }
    } else {
        ctx->power = 0xff;
        disableVoltPower();
    }


    //start the 2ms timer that controlls all the subtimers for timeouts and repeat stuff
    TMR2_Start();

    //trigger buttonbuffer to get updated twice to prevent wrong input information
    updateButtons(true);
    updateButtons(true);

    //Initialize SD-Card
    disk_initialize(0);

    //Boot display and clear it before we start the background led    
    spi1_open(DISPLAY_CONFIG);
    dispStart();
    clearScreen(COLOR_BLACK);
    spi1_close();

    //Start background led
    OC1_SecondaryValueSet(OC1_TOPVALUE);
    dispSetBrightness(device_options.brightness);
    OC1_Start();

    //reinitialize usb
    if (wakeFromSleep) {
        USBDeviceInit();
    }
    disableBUSCharge();

    return true;
}

void shutdownPeripherals(APP_CONTEXT *ctx) {
    spi_fat_open();
    unmountFS(ctx);
    spi_fat_close();

    OC1_Stop();
    _LATB2 = 1; //OC1 Pin set to high 

    SDCard_CS_SetLow();
    CS_D_SetLow();
    RS_D_SetLow();
    PER_PWR_SetHigh(); //Unpower Peripherie    
    SCLK_SetLow();
    MOSI_SetLow();
    RES_D_SetLow();

    MISO_SetWPUOff();
    MISO_SetWPDOn();

    disableVoltPower();

    TMR2_Stop();
    CRYCONLbits.CRYON = 0;

    //disableVoltPower();
    //ADC1_Disable();    //Stop Analogconverter    
    AD1CON1bits.SAMP = 0;
    AD1CON1bits.ADON = 0; // Turn off the ADC module 

    disk_deinit();

    disablePullups();
}

void setSleep(APP_CONTEXT *ctx) {
    //Shutdown all Peripherals and set the device in a "safe" state
    bool _setSleep = false;

    if (!isPinSet()) {
        swipeKeys();
    }

    if (isKeySet() && !isKeyEncr()) {
        //Encrypt masterkey in memory and store the encryption key in the crykey register.
        //This way the masterkey can be restored after wakeup and successfull pin entry
        //see function hctxVerifyKeyAfterPin(APP_CONTEXT* ctx) for unencrypt routine

        uint8_t mkey[16];
        uint8_t rndkey[16];
        uint8_t iv[16];
        uint8_t* mkeyp;

        memset(iv, 0x00, sizeof (iv));

        mkeyp = getMasterKey();
        memcpy(&mkey, mkeyp, sizeof (mkey));

        generateRND(rndkey);
        setMasterKey(rndkey);

        prepareAES128BitCBC();
        prepare128bitEncryption(iv);

        encrypt128bit(mkey, mkey);

        prepare128bitDecryption(iv);
        setMasterKey(mkey);

        setKeyEncr(true);
    }

    while (true) {
        shutdownPeripherals(ctx);
        setInitialContext(ctx);

        _setSleep = true;

        if (isBUSChargeEnabled() && !CHRG_GetValue()) {
            //Device is currently charging on USB, 
            //wait until battery is full before detach USB and proceed to full sleep mode

            __delay_ms(100);
            while (USB_BUS_SENSE && !CHRG_GetValue()) {
                if (!SENSE_CASE_GetValue()) {
                    //device reactivated, dont finish the sleep process and wakeup instead
                    _setSleep = false;
                    break;
                }
                USB_Interface_Tasks();
            }
        }

        disableBUSCharge();
        if (_setSleep) {
            USBDeviceDetach();
            USBModuleDisable();

            _LATB2 = 1;
            EX_INT0_InterruptFlagClear();
            EX_INT0_InterruptEnable();
            Sleep();
            Nop();
            EX_INT0_InterruptDisable();
            EX_INT0_InterruptFlagClear();
            __delay_ms(100);
        }

        setInitialContext(ctx);
        ctx->bus_sense = 0;
        if (bootPeripherals(ctx, _setSleep)) break;
    }

}



// ********* Extend OC1 Functions *********

uint16_t OC1_PrimaryValueGet() {
    return OC1R;
}

// ********* Extend Filesystem Functions *********

typedef struct {
    uint16_t con1;
    uint16_t brg;
    uint8_t operation;
} spi1_configuration;


// ************* Extending SPI-Functions ********

void SPI1_Transmit16bitRepeated(uint16_t data, uint16_t len) {

    uint16_t dataSentCount = 0;
    uint8_t byte1 = (uint8_t) (data >> 8);
    uint8_t byte2 = (uint8_t) (data);
    uint8_t p_dummy;

    while (dataSentCount < len) {
        while (SPI1STATLbits.SPIRBF) {
        }
        SPI1BUFL = byte1;
        while (SPI1STATLbits.SPIRBE) {
        }
        p_dummy = SPI1BUFL;

        while (SPI1STATLbits.SPIRBF) {
        }
        SPI1BUFL = byte2;
        while (SPI1STATLbits.SPIRBE) {
        }
        p_dummy = SPI1BUFL;

        dataSentCount++;
    }
}

void SPI1_Transmit16bit(uint16_t data) {
    uint8_t byte1 = (uint8_t) (data >> 8);
    uint8_t byte2 = (uint8_t) (data);
    uint8_t p_dummy;

    while (SPI1STATLbits.SPIRBF) {
    }
    SPI1BUFL = byte1;
    while (SPI1STATLbits.SPIRBE) {
    }
    p_dummy = SPI1BUFL;

    while (SPI1STATLbits.SPIRBF) {
    }
    SPI1BUFL = byte2;
    while (SPI1STATLbits.SPIRBE) {
    }
    p_dummy = SPI1BUFL;
}

void SPI1_Transmit32bit(uint32_t data) {

    uint8_t p_dummy;

    uint8_t byte1 = (uint8_t) (data >> 24);
    uint8_t byte2 = (uint8_t) (data >> 16);
    uint8_t byte3 = (uint8_t) (data >> 8);
    uint8_t byte4 = (uint8_t) (data);


    while (SPI1STATLbits.SPIRBF) {
    }
    SPI1BUFL = byte1;
    while (SPI1STATLbits.SPIRBE) {
    }
    p_dummy = SPI1BUFL;

    while (SPI1STATLbits.SPIRBF) {
    }
    SPI1BUFL = byte2;
    while (SPI1STATLbits.SPIRBE) {
    }
    p_dummy = SPI1BUFL;

    while (SPI1STATLbits.SPIRBF) {
    }
    SPI1BUFL = byte3;
    while (SPI1STATLbits.SPIRBE) {
    }
    p_dummy = SPI1BUFL;

    while (SPI1STATLbits.SPIRBF) {
    }
    SPI1BUFL = byte4;
    while (SPI1STATLbits.SPIRBE) {
    }
    p_dummy = SPI1BUFL;

}

void SPI1_transmit16bitBuffer(uint16_t *dataTransmitted, uint16_t wordCount) {
    for (uint16_t i = 0; i < wordCount; i++) {
        SPI1_Transmit16bit(dataTransmitted[i]);
    }
}

// AES-Encryption and Decryption functions

void prepareAES128BitCBC() {
    CRYCONLbits.CRYON = 1;
    CRYCONHbits.KEYSRC = 0b0000; //RAM crykey  
    CRYCONLbits.CPHRSEL = 1; //AES-engine
    CRYCONHbits.KEYMOD = 0; //128 bit key
    CRYCONLbits.CPHRMOD = 1;
}


//prepare for 128bit decryption by setting iv and masterkey with current setup

bool prepare128bitEncryption(uint8_t *iv) {
    if (!isKeySet()) return false;

    CRYCONLbits.OPMOD = 0b0000; //Operation mode encryption

    memcpy((void*) &CRYKEY0, getMasterKey(), 16); //Set Masterkey
    memcpy((void*) &CRYTXTB, iv, 16); //Set initialization Vector     

    return true;
}


//prepare for 128bit decryption by setting iv and masterkey with current setup

bool prepare128bitDecryption(uint8_t *iv) {
    if (!isKeySet()) return false;

    //Generate decryption key via AES-Keyexpand
    CRYCONLbits.OPMOD = 0b0010; //Key Expand mode    
    memcpy((void*) &CRYKEY0, getMasterKey(), 16); //Set Masterkey
    CRYCONLbits.CRYGO = 0b1; //Start keygeneration
    while (CRYCONLbits.CRYGO == 0b1);

    CRYCONLbits.OPMOD = 0b0001; //Operation mode decryption        
    memcpy((void*) &CRYTXTA, iv, 16); //Set initialization Vector     

    return true;
}

uint8_t switchregister[48];
bool switchregset = false;

void switch128BitDecryptEncrypt() {

    uint8_t swreg[48];
    memcpy(swreg, switchregister, 48); //backup siwtchregister
    memcpy(switchregister, (void*) &CRYTXTA, 16); //copy crytextregister A-C into switchregister
    memcpy(switchregister + 16, (void*) &CRYTXTB, 16);
    memcpy(switchregister + 32, (void*) &CRYTXTC, 16);
    switchregset = true;

    if (CRYCONLbits.OPMOD == 0b0000) {
        //current mode encryption, switch to dencryption

        CRYCONLbits.OPMOD = 0b0010; //Key Expand mode    
        memcpy((void*) &CRYKEY0, getMasterKey(), 16); //Set Masterkey
        CRYCONLbits.CRYGO = 0b1; //Start keygeneration
        while (CRYCONLbits.CRYGO == 0b1);
        CRYCONLbits.OPMOD = 0b0001;

        memcpy((void*) &CRYTXTA, swreg, 16);
        memcpy((void*) &CRYTXTB, swreg + 16, 16);
        memcpy((void*) &CRYTXTC, swreg + 32, 16);

    } else if (CRYCONLbits.OPMOD == 0b0001) {
        //current mode decryption, switch to encryption

        CRYCONLbits.OPMOD = 0b0000; //Operation mode encryption

        memcpy((void*) &CRYKEY0, getMasterKey(), 16); //Set Masterkey
        memcpy((void*) &CRYTXTA, swreg, 16);
        memcpy((void*) &CRYTXTB, swreg + 16, 16);
        memcpy((void*) &CRYTXTC, swreg + 32, 16);
    }
}

//encrypt a block of 128 bit with the current setup

void encrypt128bit(uint8_t *plaintext, uint8_t* ciphertext) {
    memcpy((void*) &CRYTXTA, plaintext, 16);
    CRYCONLbits.CRYGO = 1;
    while (CRYCONLbits.CRYGO == 1);
    memcpy(ciphertext, (void*) &CRYTXTB, 16);
}

//decrypt a block of 128 bit with the current setup

void decrypt128bit(uint8_t* ciphertext, uint8_t *plaintext) {
    memcpy((void*) &CRYTXTB, ciphertext, 16);
    CRYCONLbits.CRYGO = 1;
    while (CRYCONLbits.CRYGO == 1);
    memcpy(plaintext, (void*) &CRYTXTC, 16);
}

//clear keyram and stop cryptoengine

void endEncryption() {
    for (int i = 0; i < 16; i++) {
        ((uint8_t*) & CRYKEY0)[i] = 0x00;
        ((uint8_t*) & CRYTXTA)[i] = 0x00;
        ((uint8_t*) & CRYTXTB)[i] = 0x00;
        ((uint8_t*) & CRYTXTC)[i] = 0x00;
    }

    CRYCONLbits.CRYON = 0;
    if (switchregset) {
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
    while (CRYCONLbits.CRYGO == 1) {
    } /* Wait until the module is done */

    /* The random number is now located in CRYTXTA. */
    memcpy(rnd, (void*) &CRYTXTA, 16);

    /* Disable cryon if it was disabled before */
    CRYCONLbits.CRYON = con;
}

void TMR2_CallBack(void) {
    // 2ms Timer callback    
    UINT n;

    FatTimerCallback();

    //this are all 500HZ timers with zero stop
    n = gameTimer;
    if (n) gameTimer = --n;
    n = buttonTimer;
    if (n) buttonTimer = --n;
    n = adcTimer;
    if (n) adcTimer = --n;
    n = usbTimeout;
    if (n) usbTimeout = --n;
    n = keyTimeout;
    if (n) keyTimeout = --n;
    n = busChargeStartTimer;
    if (n) busChargeStartTimer = --n;
}