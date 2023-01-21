/*
 * File:   logic.c
 * Author: ronal
 *
 * Created on 20. Januar 2023, 16:05
 */

#include "xc.h"
#include "logic.h"
#include "buttons.h"
#include <string.h>

#include "mcc_ext.h"
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

/*#include "usb/usb.h"
#include "usb/usb_device_hid.h"
#include "usb/usb_tasks.h" */


//static FATFS drive;
//static FIL file;

static uint8_t pin[PIN_SIZE];

uint16_t getContextTypeLen( CONTEXT_TYPE type ) {
    switch (type) {
        case PIN_INPUT:
            return sizeof(CTX_PIN_INPUT);    
        case KEY_INPUT:
            return sizeof(CTX_KEY_INPUT);                
        case KEY_OVERVIEW:
            return sizeof(CTX_KEY_OVERVIEW); 
        default:
            return 0;
    }                    
}


bool removeContext( APP_CONTEXT *ctx ) {
    if ( ctx->ctxptr == 0 ) return false;
    
    ctx->ctxptr -= 1;
    ctx->ctxtype = ctx->ctxbuffer[ctx->ctxptr];    
    ctx->ctxptr -= getContextTypeLen( ctx->ctxtype );
    memset( ctx->ctxbuffer, 0x00, getContextTypeLen( ctx->ctxtype ) );    
    ctx->rinf = REFRESH;    
    
    return true;
}


bool pushContext( APP_CONTEXT *ctx, CONTEXT_TYPE type ) {
    uint8_t ct = type;
    uint16_t len = getContextTypeLen( type );
    uint16_t currlen = getContextTypeLen( ctx->ctxtype );
    
    if ( ( ctx->ctxptr + currlen + len + 3 ) > CTX_BUFFER_SIZE ) {
       ctx->ctxtype = ERROR;
       return false;
    }
    
    ctx->ctxptr += currlen;    
    ctx->ctxbuffer[ctx->ctxptr] = ct;
    ctx->ctxptr += 1;
    ctx->ctxtype = type;
    memset( ctx->ctxbuffer, 0x00, len );    
    ctx->rinf = REFRESH;    
        
    return true;
}


bool replaceContext( APP_CONTEXT *ctx, CONTEXT_TYPE type ) {
    removeContext( ctx );
    return pushContext( ctx, type );
}


void setContext( APP_CONTEXT *ctx, CONTEXT_TYPE type ) {
    ctx->ctxptr = 0;
    ctx->ctxtype = type;
    ctx->rinf = REFRESH;    
    memset( ctx->ctxbuffer, 0x00, getContextTypeLen( type ) );    
}


void setInitialContext( APP_CONTEXT* ctx ) {        
    ctx->ctxptr = 0;
    ctx->ctxtype = INITIAL;
    ctx->rinf = UNCHANGED;  
}


void updateContext( APP_CONTEXT* ctx ) {    
    updateButtons(false);

    switch ( ctx->ctxtype ) {
        case INITIAL: {
            if ( isKeySet() ) {            
                setContext( ctx, PIN_INPUT );
            } else {
                setContext( ctx, KEY_INPUT );
            }
        } break;
        
        
        case KEY_INPUT: {
            CTX_KEY_INPUT *sctx;
            sctx = (CTX_KEY_INPUT*)( ctx->ctxbuffer+ctx->ctxptr );

            sctx->oldx = sctx->x;
            sctx->oldy = sctx->y;
            
            if ( isButtonPressed(BUTTON_A) ) {               
                sctx->newkey[sctx->kpos] = ( sctx->y * 4 + sctx->x );
                sctx->kpos++;
                if ( sctx->kpos == 32 ) {
                    int j = 0;
                    for ( int i=0;i<32;i++) {
                        if ( i % 2 == 0 ) {
                            sctx->newkey[j] = sctx->newkey[i] << 4;
                        } else {
                            sctx->newkey[j] = sctx->newkey[j] | sctx->newkey[i];
                            j++;
                        }
                    }
                    setMasterKey( sctx->newkey );
                    if ( verifyMasterKey() ) {
                        setContext( ctx, PIN_INPUT );
                        CTX_PIN_INPUT *pctx;
                        pctx = (CTX_PIN_INPUT*)( ctx->ctxbuffer+ctx->ctxptr );
                        pctx->generatenew = 1;
                    }
                }                                
            } else if ( isButtonPressed(BUTTON_B) ) {
                sctx->kpos = sctx->kpos > 0 ? sctx->kpos-1 : 0;
                ctx->rinf = ANIMATION;
            }else if ( isButtonPressed(BUTTON_LEFT) ) {                   
                sctx->x = sctx->x > 0 ? sctx->x-1 : 0;
            } else if ( isButtonPressed(BUTTON_RIGHT) ) { 
                ctx->rinf = ANIMATION;
                sctx->x = sctx->x < 3 ? sctx->x+1 : sctx->x;
            } else if ( isButtonPressed(BUTTON_UP) ) {  
                ctx->rinf = ANIMATION;
                sctx->y = sctx->y > 0 ? sctx->y+1 : 0;
            } else if ( isButtonPressed(BUTTON_DOWN) ) {                
                ctx->rinf = ANIMATION;
                sctx->y = sctx->y < 3 ? sctx->y+1 : sctx->y;
            }
            
            
/*            for ( uint8_t i=0;i<BUTTON_COUNT_PRESSABLE; i++) {
                if ( isButtonPressed(i) ) {
                    sctx->pin[ sctx->ppos ] = i;
                    sctx->ppos++;
                    ctx->rinf = ONBUTTON_PRESS;
                    if ( sctx->ppos == PIN_SIZE ) {
                        if ( memcmp( sctx->pin, pin, PIN_SIZE ) == 0 ) {
                            //TODO: Check masterkey integrity
                            
                            setContext( ctx, KEY_OVERVIEW );
                        } else {
                            ctx->rinf = REFRESH;
                            sctx->ppos = 0;
                        }
                    }
                }
            }             */
        } break;        
                
        case PIN_INPUT: {
            CTX_PIN_INPUT *sctx;
            sctx = (CTX_PIN_INPUT*)( ctx->ctxbuffer+ctx->ctxptr );
            
            for ( uint8_t i=0;i<BUTTON_COUNT_PRESSABLE; i++) {
                if ( isButtonPressed(i) ) {
                    sctx->pin[ sctx->ppos ] = i;
                    sctx->ppos++;
                    ctx->rinf = ONBUTTON_PRESS;
                    if ( sctx->ppos == PIN_SIZE ) {
                        if ( memcmp( sctx->pin, pin, PIN_SIZE ) == 0 ) {
                            //TODO: Check masterkey integrity
                            
                            setContext( ctx, KEY_OVERVIEW );
                        } else {
                            ctx->rinf = REFRESH;
                            sctx->ppos = 0;
                        }
                    }
                }
            }
            
        } break;
        
    }
                               
}


/*  Test stuff

    if ( isButtonReleased( BUTTON_LEFT ) ) {
        setSleep();
        setInitialContext( ctx );
    }    
        
        
        //USB_Interface_Tasks();

        

         //fillRect(x, 0, 10, 4, ST7735_WHITE);

        
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


    if (ADC1_IsConversionComplete(VOLTAGE)) {
        ADC1_SoftwareTriggerDisable();
        uint16_t val = ADC1_ConversionResultGet(VOLTAGE);

        uint8_t len = (val >> 4) + 1;
        fillRect(len + 1, 30, (159 - len), 10, ST7735_BLACK);
        fillRect(0, 30, len, 10, ST7735_BLUE);

    }    
*/  