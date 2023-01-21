/*
 * File:   ui.c
 * Author: ronal
 *
 * Created on 20. Januar 2023, 16:08
 */


#include "xc.h"
#include "logic.h"
#include "display_driver.h"
#include "assets.h"
#include "mcc_generated_files/spi1_driver.h"

void renderUI(APP_CONTEXT* ctx) {
    spi1_open(DISPLAY_CONFIG);
    
    switch ( ctx->ctxtype ) {
        case KEY_INPUT: {
            switch ( ctx->rinf ) {
            
            
            }            
        } break;        
        case PIN_INPUT: {
            switch ( ctx->rinf ) {
                case REFRESH: {
                    clearScreen( ST7735_BLACK );
                    locate(0,50);
                    cWriteTextIntern( (const uint8_t*)( textdata+texte[ TEXT_ENTER_PIN ] ) );                 
                    locate(0,56);
                } break;
                case ONBUTTON_PRESS: {
                    writeText( "*" );
                } break;                    
                default:
                    break;
            }        
        } break;
                
    }
          
    ctx->rinf = UNCHANGED;
    
    spi1_close();
}