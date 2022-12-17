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
#include "usb/usb.h"
#include "usb/usb_device_hid.h"
#include "usb/usb_tasks.h"
/*
                         Main application
 */
int main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    TRISBbits.TRISB6 = 1;
    USBDeviceInit();
    USBDeviceAttach();

    while (1)
    {
        #if defined(USB_POLLING)
        /* Check bus status and service USB interrupts.  Interrupt or polling
         * method.  If using polling, must call this function periodically.
         * This function will take care of processing and responding to SETUP
         * transactions (such as during the enumeration process when you first
         * plug in).  USB hosts require that USB devices should accept and
         * process SETUP packets in a timely fashion.  Therefore, when using
         * polling, this function should be called regularly (such as once every
         * 1.8ms or faster** [see inline code comments in usb_device.c for
         * explanation when "or faster" applies])  In most cases, the
         * USBDeviceTasks() function does not take very long to execute
         * (ex: <100 instruction cycles) before it returns. */
        USBDeviceTasks();
        #endif

        /* Run the keyboard demo tasks. */
        USB_Interface_Tasks();
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