/*
 * File:   logic.c
 * Author: greengravity
 *
 * Created on 20. Januar 2023, 16:05
 */

#include "xc.h"
#include "logic.h"
#include "buttons.h"
#include <string.h>

#include "mcc_ext.h"
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/pin_manager.h"
#include "mcc_generated_files/spi1_driver.h"
#include "mcc_generated_files/oc1.h"
#include "mcc_generated_files/spi1_driver.h"
#include "mcc_generated_files/spi1_types.h"
#include "mcc_generated_files/adc1.h"
#include "mcc_generated_files/ext_int.h"
#include "mcc_generated_files/interrupt_manager.h"

#include "fs/diskio.h"
#include "fs/ff.h"

/*#include "usb/usb.h"
#include "usb/usb_device_hid.h"
#include "usb/usb_tasks.h" */


//static FATFS drive;
//static FIL file;

static uint8_t pin[PIN_SIZE];
static uint8_t pinerr = 0;

static uint8_t masterkey[16];
static bool keyset = false;

//128bit AES key

const TOKEN_CONFIG token_configs[] = {
    { NEW, TEXT_IOHEAD_NEW },
    { NAME, TEXT_IOHEAD_NAME },
    { KEY, TEXT_IOHEAD_KEY },
    { KEY2, TEXT_IOHEAD_KEY2 },
    { URL, TEXT_IOHEAD_URL },
    { INFO, TEXT_IOHEAD_INFO }        
};



bool isKeySet() {
    return keyset;
}

void setMasterKey(uint8_t *key) {
    keyset = true;
    memcpy(masterkey, key, 16);
}

void setMasterTestKey( ) {
    uint8_t testkey[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};        
    setMasterKey(testkey);    
}

uint8_t* getMasterKey() {
    return masterkey;
}

void swipeKeys() {
    memset(masterkey, 0x00, 16);
    memcpy((void*) &CRYKEY0, masterkey, 16);
    keyset = false;
    memset(pin, 0x00, PIN_SIZE);
    pinerr = 0;
}

uint8_t verifyMasterKey() {        
    UINT rwbytes;
    uint8_t data[48];            
    uint8_t cipher[16];            
    uint8_t iv[16];

    FIL file;
    FILINFO fno;
    
    uint8_t result = 0;
    
    if ( f_stat( "VERIFY", &fno ) == FR_OK && f_open(&file, "VERIFY", FA_READ ) == FR_OK ) {                        
        if ( f_read(&file, data,48, &rwbytes) == FR_OK && rwbytes == 48 ) {
            memset(&iv, 0x00, 16);

            //decode data
            prepareAES128BitCBC();                    
            prepare128bitDecryption( iv );

            for (int i=0;i<3;i++) {
                decrypt128bit( data+(i*16), cipher );
                memcpy( data+(i*16), cipher, 16 );
            }                    

            for (int i=0;i<16;i++) {
               cipher[i] = data[i] ^ data[i+16];
            }
            if ( memcmp( data+32, cipher, 16 ) == 0 ) {
                result = 0;
            } else {
                result = 1;
            }      
        } else {
            result = 2;
        }                                                
    } else {

        if ( f_open(&file, "VERIFY", FA_WRITE | FA_CREATE_ALWAYS ) == FR_OK ) {
        
            //create new verifikation file
            generateRND(data);
            generateRND(data+16);
            for (int i=0;i<16;i++) {
                data[i+32] = data[i] ^ data[i+16];
                iv[i] = 0x00;
            }

            //encrypt all 3 blocks
            prepareAES128BitCBC();            
            prepare128bitEncryption( iv );

            for (int i=0;i<3;i++) {
                encrypt128bit( data+(i*16), cipher );
                memcpy( data+(i*16), cipher, 16 );
            }

            endEncryption( );

            //after encryption, the 3rd block should only calculate right if the correct key is used
            if ( f_write(&file, data, 48, &rwbytes) == FR_OK && rwbytes == 48 ) {               
                result = 0;
            } else {
                result = 2;   
            }  
            f_close(&file);
        } else {
            result = 2;
        }        
    }
       
    if ( result ==  0 ) {
        if ( f_stat("KS", &fno) != FR_OK ) {
            //create Keystore directory if not exits
            f_mkdir( "KS" );
            if ( f_stat("KS", &fno) == FR_OK && ( fno.fattrib & AM_DIR ) ) {
                result = 0;
            } else {
                result = 2;
            }
        }
    } 
    
    return result;
}

uint16_t _getContextTypeLen(CONTEXT_TYPE type) {
    switch (type) {
        case ERROR:
            return sizeof (CTX_ERROR);
        case PIN_INPUT:
            return sizeof (CTX_PIN_INPUT);
        case KEY_INPUT:
            return sizeof (CTX_KEY_INPUT);
        case ENTRY_OVERVIEW:
            return sizeof (CTX_ENTRY_OVERVIEW);
        case NEW_ENTRY:
            return sizeof(CTX_NEW_ENTRY);
        default:
            return 0;
    }
}

uint16_t getContextTypeLen(CONTEXT_TYPE type) {
    uint16_t len =  _getContextTypeLen(type);
    if ( len % 2 != 0 ) len ++;
    return len;
}

bool removeContext(APP_CONTEXT *ctx) {
    if (ctx->ctxptr <= 1 ) {
        if ( ctx->ctxtype != EMPTY ) {
            ctx->ctxtype = EMPTY;
            return true;
        }      
        return false;
    }
    
    ctx->ctxptr -= 2;
    ctx->ctxtype = ctx->ctxbuffer[ctx->ctxptr];
    ctx->ctxptr -= getContextTypeLen(ctx->ctxtype);
    ctx->rinf = REFRESH;

    return true;
}

bool pushContext(APP_CONTEXT *ctx, CONTEXT_TYPE type) {    
    uint16_t len = getContextTypeLen(type);
    uint16_t currlen = getContextTypeLen(ctx->ctxtype);
    uint8_t ct = ctx->ctxtype;
    
    ctx->rinf = REFRESH;   
    if ((ctx->ctxptr + currlen + len + 2) > CTX_BUFFER_SIZE) {
        ctx->ctxtype = ERROR_CONTEXT;         
        return false;
    }

    ctx->ctxptr += currlen;
    ctx->ctxbuffer[ctx->ctxptr] = ct;
    ctx->ctxptr += 2;
    ctx->ctxtype = type;
    memset(ctx->ctxbuffer+ctx->ctxptr, 0x00, len);
    
    return true;
}

bool replaceContext(APP_CONTEXT *ctx, CONTEXT_TYPE type) {
    removeContext(ctx);
    return pushContext(ctx, type);
}

bool setContext(APP_CONTEXT *ctx, CONTEXT_TYPE type) {
    ctx->ctxptr = 0;
    if  (  ((uint16_t)ctx->ctxbuffer) % 2 != 0 ) ctx->ctxptr++;
    
    uint16_t len = getContextTypeLen(type);
    if ( ctx->ctxptr + len > CTX_BUFFER_SIZE ) {
        ctx->ctxtype = ERROR_CONTEXT;
        ctx->rinf = REFRESH;
        return false;
    }
    
    
    ctx->ctxtype = type;
    ctx->rinf = REFRESH;          
    memset(ctx->ctxbuffer, 0x00, getContextTypeLen(type));
    
    return true;
}

void setInitialContext(APP_CONTEXT* ctx) {
    setContext(ctx, INITIAL);
}


Keylayout * getCurrentKeyboardKey( IO_CONTEXT *ioctx ) {
    Keyboardmaps* km = (Keyboardmaps*)&keymaps[ ioctx->kbmap ];    
    return (Keylayout *)&keylayouts[ioctx->kby*10+ioctx->kbx + km->layoutoff];
}

uint8_t hctxIOWriteCharacter( IO_CONTEXT *ioctx ) {
    
}

uint8_t hctxIO( IO_CONTEXT *ioctx ) {
    
    if ( ioctx->selarea == 0 ) {
        //cursor on keyboard       
        if ( isButtonPressed(BUTTON_A) ) {
            
        } else if ( isButtonReleased(BUTTON_A) ) {
            
        } else if ( isButtonDown(BUTTON_A) ) {
            //nothing to do, just block other buttons
        } else if ( isButtonPressed(BUTTON_UP) ||
             isButtonPressed(BUTTON_DOWN) ||
             isButtonPressed(BUTTON_LEFT) ||
             isButtonPressed(BUTTON_RIGHT) ) {
            
            if ( isButtonPressed(BUTTON_UP) && ioctx->kby == 0 ) {
                ioctx->rinf = AREASWITCH;
                ioctx->oselarea = ioctx->selarea;
                ioctx->selarea = 1;                                
            } else {
                ioctx->okby = ioctx->kby;
                ioctx->okbx = ioctx->kbx;                                
                ioctx->rinf = ANIMATION;

                Keylayout *k = getCurrentKeyboardKey(ioctx);
                
                if ( isButtonPressed(BUTTON_UP) && ioctx->kby > 0 ) ioctx->kby--;
                if ( isButtonPressed(BUTTON_DOWN) && ioctx->kby < 3 ) ioctx->kby++;
                if ( isButtonPressed(BUTTON_LEFT) && ioctx->kbx > k->span_neg ) ioctx->kbx -= ( k->span_neg + 1 );
                if ( isButtonPressed(BUTTON_RIGHT) && ioctx->kbx < (9 - k->span_pos) ) ioctx->kbx += ( k->span_pos + 1 );
            }                                    
        } else if ( isButtonPressed( BUTTON_B) ) {
            ioctx->oselarea = ioctx->selarea;
            ioctx->selarea = 2; //open keymap
            ioctx->rinf = AREASWITCH;          
        }
   
    } else if ( ioctx->selarea == 1 ) {
        //cursor in textarea
        if ( isButtonPressed(BUTTON_DOWN) ) {
            ioctx->rinf = AREASWITCH;
            ioctx->oselarea = ioctx->selarea;
            ioctx->selarea = 0; //back to keyboard
        }
        
    } else if ( ioctx->selarea == 2 ) {
        if ( isButtonPressed(BUTTON_UP) ||
             isButtonPressed(BUTTON_DOWN) || 
             isButtonPressed(BUTTON_LEFT) ||
             isButtonPressed(BUTTON_RIGHT) )
        {
            int x = ioctx->kbmap % 3;
            int y = ioctx->kbmap / 3;
            
            if ( isButtonPressed(BUTTON_LEFT) && x > 0 )  x--;           
            if ( isButtonPressed(BUTTON_RIGHT) && x < 2 ) x++;
            if ( isButtonPressed(BUTTON_UP) && y > 0 )  y--;           
            if ( isButtonPressed(BUTTON_DOWN) && y < 2 ) y++;
            
            int newmap = y*3+x;
            
            if ( keymaps[newmap].active ) {
                ioctx->okbmap = ioctx->kbmap;
                ioctx->kbmap = newmap;
                ioctx->rinf = ANIMATION;
            }
                                       
        } else if ( isButtonPressed(BUTTON_A) ||
                    isButtonPressed(BUTTON_B) ) {
            
            ioctx->oselarea = ioctx->selarea;
            ioctx->selarea = 0; //back to keyboard            
            ioctx->rinf = AREASWITCH;
        }        
    }
    
    if ( ioctx->rinf == ANIMATION || ioctx->rinf == AREASWITCH ) {
        return 1;
    }
      
/*    } else if ( res == 1 ) {
        ctx->rinf = IO_UPDATE;      */
    
    
    return 0;
}


void hctxPinInput(APP_CONTEXT* ctx) {
    CTX_PIN_INPUT *sctx;
    sctx = (CTX_PIN_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);

    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        if (isButtonPressed(i)) {
            if (sctx->verify) {
                sctx->verifypin[ sctx->ppos ] = i;
            } else {
                sctx->pin[ sctx->ppos ] = i;
            }
            sctx->ppos++;
            ctx->rinf = ONBUTTON_PRESS;
            sctx->haderror = sctx->error;
            sctx->error = false;

            if (sctx->ppos == PIN_SIZE) {
                if (sctx->generatenew) {
                    if (sctx->verify) {
                        if (memcmp(sctx->pin, sctx->verifypin, PIN_SIZE) == 0) {
                            //Pin match verification pin, set new pin and go to overview page
                            memcpy(pin, sctx->pin, PIN_SIZE);
                            pinerr = 0;
                            setContext(ctx, ENTRY_OVERVIEW);
                        } else {
                            //Pin differs from verification pin, retry
                            sctx->error = true;
                            sctx->verify = false;
                            sctx->ppos = 0;
                            memset(sctx->pin, 0x00, PIN_SIZE);
                            memset(sctx->verifypin, 0x00, PIN_SIZE);
                        }
                    } else {
                        sctx->verify = true;
                        sctx->ppos = 0;

                        ctx->rinf = ANIMATION;
                    }
                } else {
                    if (memcmp(sctx->pin, pin, PIN_SIZE) == 0) {
                        //TODO: Check masterkey integrity
                        pinerr = 0;
                        setContext(ctx, ENTRY_OVERVIEW);
                    } else {
                        ctx->rinf = REFRESH;
                        sctx->ppos = 0;
                        pinerr++;
                        if (pinerr == MAX_PIN_TRIES) {
                            pinerr = 0;
                            setContext(ctx, KEY_INPUT);
                        }
                    }
                }
            }
            break;
        }
    }

}

void hctxKeyInput(APP_CONTEXT* ctx) {
    CTX_KEY_INPUT *sctx;
    sctx = (CTX_KEY_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);

    sctx->oldx = sctx->x;
    sctx->oldy = sctx->y;
    sctx->haderror = sctx->error;

    if (isButtonPressed(BUTTON_A)) {
        //add the selected char as half-byte to the newkey
        sctx->newkey[sctx->kpos] = (sctx->y * 4 + sctx->x);
        sctx->kpos++;
        if (sctx->kpos == 32) {
            //condense the 1 byte per character to 1 byte per 2 character to obtain a valid encryption key
            int j = 0;
            for (int i = 0; i < 32; i++) {
                if (i % 2 == 0) {
                    sctx->newkey[j] = sctx->newkey[i] << 4;
                } else {
                    sctx->newkey[j] = sctx->newkey[j] | sctx->newkey[i];
                    j++;
                }
            }
            //set and verify the inputed masterkey
            setMasterKey(sctx->newkey);            
            uint8_t keyres = verifyMasterKey();
            if ( keyres == 0 ) {
                setContext(ctx, PIN_INPUT);
                CTX_PIN_INPUT *pctx;
                pctx = (CTX_PIN_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);
                pctx->generatenew = 1;
            } else if ( keyres == 1 ) {
                sctx->kpos = 0;
                sctx->error = true;
                ctx->rinf = REFRESH;
            } else if ( keyres == 2 ) {
                setContext(ctx, ERROR_SD_FAILURE);
            }
        } else {
            sctx->error = false;
            ctx->rinf = ONBUTTON_PRESS;
        }
    } else if (isButtonReleased(BUTTON_A)) {
        if (sctx->kpos > 0) {
            ctx->rinf = ONBUTTON_RELEASE;
        }
    } else if (isButtonDown(BUTTON_A)) {
        //no further handling
    } else if (isButtonPressed(BUTTON_B)) {
        sctx->kpos = sctx->kpos > 0 ? sctx->kpos - 1 : 0;
        ctx->rinf = REMOVECHAR;
        sctx->error = false;
    } else if (isButtonPressed(BUTTON_LEFT)) {
        //left
        sctx->x = sctx->x > 0 ? sctx->x - 1 : 0;
        ctx->rinf = ANIMATION;
        sctx->error = false;
    } else if (isButtonPressed(BUTTON_RIGHT)) {
        //right
        sctx->x = sctx->x < 3 ? sctx->x + 1 : sctx->x;
        ctx->rinf = ANIMATION;
        sctx->error = false;
    } else if (isButtonPressed(BUTTON_DOWN)) {
        //up
        sctx->y = sctx->y < 3 ? sctx->y + 1 : sctx->y;
        ctx->rinf = ANIMATION;
        sctx->error = false;
    } else if (isButtonPressed(BUTTON_UP)) {
        //down
        sctx->y = sctx->y > 0 ? sctx->y - 1 : 0;
        ctx->rinf = ANIMATION;
        sctx->error = false;
    }
}


void hctxEntryOverview(APP_CONTEXT* ctx) {
    if ( isButtonPressed(BUTTON_A) ) {        
        if ( replaceContext(ctx, NEW_ENTRY ) ) {
            CTX_NEW_ENTRY *sctx;
            sctx = (CTX_NEW_ENTRY*) (ctx->ctxbuffer + ctx->ctxptr);
            sctx->io.type = NEW;
            sctx->io.rinf = REFRESH;
            sctx->io.kbmap = DEFAULT_KEYMAP;
            ctx->rinf = IO_UPDATE;
        }
    }     
}

void hctxNewEntry(APP_CONTEXT* ctx) {
    CTX_NEW_ENTRY *sctx;
    sctx = (CTX_NEW_ENTRY*) (ctx->ctxbuffer + ctx->ctxptr);                       
    
    uint8_t res = hctxIO( &sctx->io );    
    if ( res == 0 ) {
        ctx->rinf = UNCHANGED;
    } else if ( res == 1 ) {
        ctx->rinf = IO_UPDATE;        
    } else if ( res == 2 ) {
        //Input done                
    } else if ( res == 3 ) {
       //Input aborted           
    }
}



void updateContext(APP_CONTEXT* ctx) {
    updateButtons(false);
    if (SENSE_CASE_GetValue()) {
        // Sleep
        setSleep(ctx);
        if ( !ctx->fsmounted ) {
            setContext(ctx, ERROR_SD_CD);
        }
    }
    
    
    spi_fat_open(); //Open FAT SPI while handle logic 
    
    if (SDCard_CD_GetValue()) {
        //SD-Card removed       
        if (ctx->ctxtype != ERROR_SD_CD) {
            ctx->fsmounted = false;
            setContext(ctx, ERROR_SD_CD);
        }
    } else {
        //SD-Card inserted
        if (ctx->ctxtype == ERROR_SD_CD) {
            mountFS( ctx );
            if ( ctx->fsmounted ) {
                setContext(ctx, INITIAL);
            }
        }
    }


    switch (ctx->ctxtype) {
        case INITIAL:
        {
            if (isKeySet()) {
                setContext(ctx, PIN_INPUT);
            } else {
                setContext(ctx, KEY_INPUT);
            }
                    
            setMasterTestKey( );
            setContext(ctx, ENTRY_OVERVIEW);                     
        }
            break;

        case KEY_INPUT:        
            hctxKeyInput(ctx);        
            break;

        case PIN_INPUT:        
            hctxPinInput(ctx);        
            break;
        case ENTRY_OVERVIEW:
            hctxEntryOverview(ctx);        
            break;
        case NEW_ENTRY:
            hctxNewEntry(ctx);        
            break;            
    }
    
    spi_fat_close(); //Close FAT SPI while handle logic 
    
    /*
    if ( ctx->fsmounted ) {
        //in case of open drive/file set filesystem to suspend
        if ( ctx->fileopen ) {
            fs_standby(ctx->file);
        } else {
            fs_standby( );
        }        
    } 
    */           
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