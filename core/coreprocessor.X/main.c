/**
  Generated main.c file from MPLAB Code Configurator

  @Company
    Microchip Technology Inc.

  @File Name
    main.c

  @Summary
    This is the generated main.c using PIC24 / dsPIC33 / PIC32MM MCUs.

  @Description
    This source file provides main entry point for system initialization and application code development.
    Generation Information :
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.171.1
        Device            :  PIC24FJ128GB204
    The generated drivers are tested against the following:
        Compiler          :  XC16 v1.70
        MPLAB 	          :  MPLAB X v5.50
 */

/*
    (c) 2020 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
 */

/**
  Section: Included Files
 */
#include "main.h"
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/fatfs/fatfs_demo.h"
#include "mcc_generated_files/pin_manager.h"
#include "mcc_generated_files/spi1_driver.h"
#include "mcc_generated_files/oc1.h"
#include "mcc_generated_files/spi1_driver.h"
#include "mcc_generated_files/spi1_types.h"
#include "mcc_generated_files/adc1.h"
#include "mcc_generated_files/fatfs/ff.h"
#include "mcc_generated_files/ext_int.h"
#include "mcc_generated_files/interrupt_manager.h"
#include "mcc_ext.h"


/*#include "usb/usb.h"
#include "usb/usb_device_hid.h"
#include "usb/usb_tasks.h" */
#include "display_driver.h"
#include "buttons.h"
//#include "file_ext.h"


static FATFS drive;
static FIL file;
volatile bool intset=false;

#define C_PRIM_VALUE_STEP 1

void bootPeripherals() {
    
    PWR_PER_SetLow();
    SENSE_SetLow();
    __delay_ms(150);

    spi1_open(DISPLAY_CONFIG);
    dispStart();
    spi1_close();
  
/*    
    ADC1_ChannelSelect(VOLTAGE);
    ADC1_Enable();
    ADC1_SoftwareTriggerDisable();
  */
    
    updateButtons(true);
    __delay_ms(50);
    updateButtons(true);
}

void shutdownPeripherals() {   
    dispStop();
    spi1_close();
    SDCard_CS_SetHigh();
    PWR_PER_SetHigh(); //Unpower Peripherie
    SENSE_SetHigh();   //Unpower Voltagedivider
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
    
    //__delay_ms(5000);
    
    EX_INT0_InterruptDisable();
    EX_INT0_InterruptFlagClear();
    __delay_ms(100);
    
    bootPeripherals();
}


int main(void) {
    SYSTEM_Initialize();    
    EX_INT0_InterruptDisable();
    bootPeripherals();
    
    while (1) {
                
        updateButtons(false);
        //USB_Interface_Tasks();

        spi1_open(DISPLAY_CONFIG);

        uint8_t x = 0;

        //fillRect(x, 0, 10, 4, ST7735_WHITE);

/*        
        if (isButtonPressed(BUTTON_LEFT)) {

            fillRect(x, 10, 10, 10, ST7735_RED);
            
            spi1_close();

            __delay_ms(100);

            if (f_mount(&drive, "0:", 1) == FR_OK) {
                if (f_open(&file, "HELLO.TXT", FA_WRITE | FA_CREATE_ALWAYS ) == FR_OK) {
                    fs_standby_file(&file);
                    writing = true;                    
                } else {
                }
            } else {
            }

 
            spi1_open(DISPLAY_CONFIG);

            fillRect(x, 10, 10, 10, ST7735_BLACK);

        } else if ( isButtonDown(BUTTON_LEFT) ) {
            if ( writing ) {
                spi1_close();

                char data[] = "Next!";
                UINT actualLength;
                fs_resume();
                f_write(&file, data, sizeof (data) - 1, &actualLength);                    
                fs_standby(&file);
                
                spi1_open(DISPLAY_CONFIG);

                fillRect(x, 10, 10, 10, ST7735_BLUE);                
            }            
        } else if ( isButtonReleased( BUTTON_LEFT ) ) {
            spi1_close();
            
            fs_resume();            
            f_close(&file);
            f_mount(0, "0:", 0);
            __delay_ms(10);

            spi1_open(DISPLAY_CONFIG);
                        
            fillRect(x, 10, 10, 10, ST7735_BLACK);
        }
*/
        x = 0;
        fillRect(x, 0, 10, 4, ST7735_WHITE);
        if (isButtonDown(BUTTON_LEFT)) fillRect(x, 10, 10, 10, ST7735_RED);
        else fillRect(x, 10, 10, 10, ST7735_BLACK);

        x += 15;
        fillRect(x, 0, 10, 4, ST7735_WHITE);
        if (isButtonDown(BUTTON_UP)) {

            if (dispGetBrightness() >= C_PRIM_VALUE_STEP) {
                dispSetBrightness( dispGetBrightness() - C_PRIM_VALUE_STEP ); 
            } else dispSetBrightness( 0 ); 

            fillRect(x, 10, 10, 10, ST7735_RED);
        } else fillRect(x, 10, 10, 10, ST7735_BLACK);

        x += 15;
        fillRect(x, 0, 10, 4, ST7735_WHITE);
        if (isButtonDown(BUTTON_RIGHT)) {

            if ( dispGetBrightness() <= 100 - C_PRIM_VALUE_STEP) {                
                dispSetBrightness( dispGetBrightness() + C_PRIM_VALUE_STEP ); 
            } else dispSetBrightness( 100 );             

            fillRect(x, 10, 10, 10, ST7735_RED);
        } else fillRect(x, 10, 10, 10, ST7735_BLACK);

        x += 15;
        fillRect(x, 0, 10, 4, ST7735_WHITE);
        if (isButtonDown(BUTTON_DOWN)) fillRect(x, 10, 10, 10, ST7735_RED);
        else fillRect(x, 10, 10, 10, ST7735_BLACK);

        x += 15;
        fillRect(x, 0, 10, 4, ST7735_WHITE);
        if (isButtonDown(BUTTON_A)) fillRect(x, 10, 10, 10, ST7735_RED);
        else fillRect(x, 10, 10, 10, ST7735_BLACK);

        x += 15;
        fillRect(x, 0, 10, 4, ST7735_WHITE);
        if (isButtonDown(BUTTON_B)) fillRect(x, 10, 10, 10, ST7735_RED);
        else fillRect(x, 10, 10, 10, ST7735_BLACK);

        x += 15;
        fillRect(x, 0, 10, 4, ST7735_WHITE);
        if ( intset ) fillRect(x, 10, 10, 10, ST7735_RED);
        else fillRect(x, 10, 10, 10, ST7735_BLACK);
        
/*        
        if (ADC1_IsConversionComplete(VOLTAGE)) {
            ADC1_SoftwareTriggerDisable();
            uint16_t val = ADC1_ConversionResultGet(VOLTAGE);

            uint8_t len = (val >> 4) + 1;
            fillRect(len + 1, 30, (159 - len), 10, ST7735_BLACK);
            fillRect(0, 30, len, 10, ST7735_BLUE);

        }
  */
        

        locate(10, 80);
        writeText("OK Bro");
    
        spi1_close();
        
        if ( isButtonReleased( BUTTON_LEFT ) ) {
            setSleep();
        }
         

    }


    return 1;
}


 void EX_INT0_CallBack()
{
}


/*
#if defined(USB_INTERRUPT)

void __attribute__((interrupt, auto_psv)) _USB1Interrupt() {
    USBDeviceTasks();
}
#endif
*/


