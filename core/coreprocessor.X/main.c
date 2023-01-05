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
#include "mcc_generated_files/pin_manager.h"
#include "usb/usb.h"
#include "usb/usb_device_hid.h"
#include "usb/usb_tasks.h"
#include "display/display_driver.h"
#include "mcc_generated_files/spi1.h"
#include "mcc_generated_files/oc1.h"
#include "images/sysimages.h"



int main(void)
{
    uint16_t primvalue = 0x0088;
    uint16_t secvalue = 0x00FF;
    
    // initialize the device
    SYSTEM_Initialize();
    TRISBbits.TRISB6 = 1;
    //USBDeviceInit();
    //USBDeviceAttach();
    SPI1_Initialize();
    
    setupDisplay( );
    clearScreen(ST7735_BLACK );
    OC1_Initialize();
    OC1_Start();
    OC1_SecondaryValueSet( secvalue );
//    startDisplay();
//    setGraphicbuffer( (uint8_t*) &graphicbuffer, 128, 0 );
//    writeDisplay();
    
    while (1)
    {
                                        
        //USB_Interface_Tasks();
        
        if ( IO_RA8_GetValue() == 0 ) {
            primvalue++;
            if ( primvalue > secvalue ) {
                primvalue = secvalue;
            }
            OC1_PrimaryValueSet( primvalue );
            __delay_ms(50);
        }

        if ( IO_RB4_GetValue() == 0 ) {
            if ( primvalue > 0 ) {
                primvalue--;                
            }
            OC1_PrimaryValueSet( primvalue );
            __delay_ms(50);
        }
        
        uint16_t len;
        len = primvalue / 2;
        
        fillRect(10,10,140,1, ST7735_BLUE);        
        fillRect(10,50,140,10, ST7735_BLACK);
        fillRect(10,50,primvalue,10, ST7735_GREEN);
                
        //drawImage( 10, 70, ST7735_BLUE, ST7735_BLACK, &gfximages[SYSTEMSYMBOLS].image[5], gfximages[SYSTEMSYMBOLS].bitmap );
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