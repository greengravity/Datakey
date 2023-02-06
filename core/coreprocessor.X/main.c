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

int main(void) {
    SYSTEM_Initialize();        

    uint8_t contextbuffer[CTX_BUFFER_SIZE];      
    APP_CONTEXT ctx;
    ctx.ctxbuffer = contextbuffer;
    ctx.fsmounted = false;
    ctx.fileopen = false;        
    
    setInitialContext(&ctx);
    bootPeripherals(&ctx);

    if ( !ctx.fsmounted ) {
        setContext(&ctx, ERROR_SD_CD);
    }
           
    while (1) {                
        updateContext(&ctx);
        renderUI(&ctx);
    }
 
    while (1);
    return 1;
}



/*
#if defined(USB_INTERRUPT)

void __attribute__((interrupt, auto_psv)) _USB1Interrupt() {
    USBDeviceTasks();
}
#endif
*/


