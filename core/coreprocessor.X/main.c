/*
 * File:   main.c
 * Author: greengravity
 *
 * Created on 14. Januar 2023, 16:01
 */

#include "main.h"
#include "logic.h"
#include "ui.h"
#include "mcc_ext.h"
#include "mcc_generated_files/system.h"

#include "mcc_generated_files/spi1_driver.h"
#include "display_driver.h"
#include "usb/usb.h"

//#include "mcc_generated_files/adc1.h"
#include "mcc_generated_files/oc1.h"
#include "mcc_generated_files/tmr2.h"

APP_CONTEXT ctx;
uint8_t contextbuffer[CTX_BUFFER_SIZE];

int main(void) {
    SYSTEM_Initialize();                      
     
    ANSB |= 0x4000; //Set pin RB14 to analog
    
    ctx.ctxbuffer = contextbuffer;
    ctx.fsmounted = false;
    ctx.fileopen = false;        
    
    device_options.brightness = 100;
    device_options.highlight_color2 = device_options.highlight_color1 = highlight_color_tab[0];
    device_options.umode = USB_MODE_KEYBOARD;
    
//    setSleep(&ctx);  //Start in sleep mode

    setInitialContext(&ctx);
    bootPeripherals(&ctx);
                      
    while (1) {
        updateContext(&ctx);
        renderUI(&ctx);
    }
 
    while (1);
    return 1;
}




#if defined(USB_INTERRUPT)

void __attribute__((interrupt, auto_psv)) _USB1Interrupt() {
    USBDeviceTasks();
}
#endif


