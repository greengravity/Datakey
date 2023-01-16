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
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/fatfs/fatfs_demo.h"
#include "mcc_generated_files/pin_manager.h"
#include "usb/usb.h"
#include "usb/usb_device_hid.h"
#include "usb/usb_tasks.h"
#include "display/display_driver.h"
#include "mcc_generated_files/spi1_driver.h"
#include "mcc_generated_files/oc1.h"
#include "images/sysimages.h"
#include "mcc_generated_files/spi1_driver.h"
#include "mcc_generated_files/spi1_types.h"
#include "mcc_generated_files/adc1.h"
#include "buttons.h"
#include "mcc_generated_files/fatfs/ff.h"


static FATFS drive;
static FIL file;

#define C_PRIM_VALUE_STEP 300

int main(void)
{
    uint16_t primvalue = 0x6F00;
    uint16_t secvalue = 0x7F00;
    
    bool sw = false;
        
    // initialize the device
    SYSTEM_Initialize();
    
    IO_RB3_SetLow();
    IO_RA7_SetLow();
    __delay_ms(150);    
    
    spi1_open( DISPLAY_CONFIG );
    setupDisplay( );
    clearScreen(ST7735_BLACK );
    spi1_close();
    
    OC1_Initialize();
    OC1_Start();
    OC1_SecondaryValueSet( secvalue );
    primvalue = 0x0000;
    OC1_PrimaryValueSet( primvalue );
    
    ADC1_ChannelSelect( BATTERY_READ );
    ADC1_Enable();
    ADC1_SoftwareTriggerDisable();
    //FatFsDemo_Tasks();
    uint16_t lastval = 256;
    
    uint8_t bp = false;
    uint8_t writing = false;
    
    while (1)
    {
             
        //USB_Interface_Tasks();
        
        sw = !sw;
        
        spi1_open( DISPLAY_CONFIG );

        uint8_t x = 0;
        
        fillRect(x,0, 10, 4, ST7735_WHITE);           
        if ( isButtonDown( BUTTON_LEFT ) ) {
                          
            spi1_close( );
            
            if ( !bp ) {
                bp = true;
/*                
                __delay_ms(10);
                
                if (f_mount(&drive,"0:",1) == FR_OK)
                {
                    if (f_open(&file, "HELLO.TXT", FA_WRITE | FA_CREATE_NEW ) == FR_OK)
                    {
                        char data[] = "Next!";
                        UINT actualLength;
                        f_write(&file, data, sizeof(data)-1, &actualLength ); 
                                             
                        f_close(&file);  
                        f_mount(0,"0:",0);                        
                        
                        writing = true;
                        __delay_ms(10);
                    } else {
                        writing = false;
                    }                  
                } else {
                    writing = false;
                }
  */                         
            } else {
                if  ( writing ) {
/*                    
                    char data[] = "Next!";
                    UINT actualLength;
                    f_write(&file, data, sizeof(data)-1, &actualLength ); 
                    f_sync (&file);
 */ 
                }               
            }

            spi1_open( DISPLAY_CONFIG );
            
            if ( writing ) {
                 fillRect(x,10, 10, 10, ST7735_YELLOW); 
             } else {
                 fillRect(x,10, 10, 10, ST7735_RED); 
             }            
            
        }
        else { 
            
            if ( bp ) {
                bp = false;                
                spi1_close( );
                __delay_ms(10);
                
                if ( writing ) {
/*                    
                    f_close(&file);  
                    f_mount(0,"0:",0);
                    __delay_ms(10);
 */
                }
                
                spi1_open( DISPLAY_CONFIG );
                
                fillRect(x,10, 10, 10, ST7735_BLACK);    
            }             
                               
        }

        x += 15;
        fillRect(x,0, 10, 4, ST7735_WHITE);           
        if ( isButtonDown( BUTTON_UP ) ) {
            
            if ( primvalue > C_PRIM_VALUE_STEP ) {
                primvalue -= C_PRIM_VALUE_STEP;                
            } else primvalue = 0;
            OC1_PrimaryValueSet( primvalue );
            
            fillRect(x,10, 10, 10, ST7735_RED);            
        }
        else fillRect(x,10, 10, 10, ST7735_BLACK);           

        x += 15;
        fillRect(x,0, 10, 4, ST7735_WHITE);           
        if ( isButtonDown( BUTTON_RIGHT ) ) {
            
            if ( primvalue < secvalue - C_PRIM_VALUE_STEP ) {
                primvalue += C_PRIM_VALUE_STEP;                
            } else primvalue = secvalue;
            OC1_PrimaryValueSet( primvalue );            
            
            fillRect(x,10, 10, 10, ST7735_RED);
        } else fillRect(x,10, 10, 10, ST7735_BLACK);           

        x += 15;
        fillRect(x,0, 10, 4, ST7735_WHITE);           
        if ( isButtonDown( BUTTON_DOWN ) ) fillRect(x,10, 10, 10, ST7735_RED);
        else fillRect(x,10, 10, 10, ST7735_BLACK);           

        x += 15;
        fillRect(x,0, 10, 4, ST7735_WHITE);           
        if ( isButtonDown( BUTTON_A ) ) fillRect(x,10, 10, 10, ST7735_RED);
        else fillRect(x,10, 10, 10, ST7735_BLACK);           

        x += 15;
        fillRect(x,0, 10, 4, ST7735_WHITE);           
        if ( isButtonDown( BUTTON_B ) ) fillRect(x,10, 10, 10, ST7735_RED);
        else fillRect(x,10, 10, 10, ST7735_BLACK);           
          
        if ( ADC1_IsConversionComplete( BATTERY_READ ) ) {
            ADC1_SoftwareTriggerDisable();
            uint16_t val = ADC1_ConversionResultGet( BATTERY_READ );
            //if ( lastval != val ) {
                lastval = val;
                uint8_t len = ( lastval >> 4 ) + 1;
                fillRect(len + 1,30, ( 159 - len ), 10, ST7735_BLACK);           
                fillRect(0,30, len, 10, ST7735_BLUE);           
            //}
        }
        
         
        drawImage( 10, 80, &bitmaps[SYSICON_OK] );
        spi1_close();
        
    }
    
    
    return 1;
}
/**
 End of File
*/

#if defined(USB_INTERRUPT)
void __attribute__((interrupt,auto_psv)) _USB1Interrupt()
{
    USBDeviceTasks();
}
#endif