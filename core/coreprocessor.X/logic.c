/*
 * File:   logic.c
 * Author: greengravity
 *
 * Created on 20. Januar 2023, 16:05
 */

#include "main.h"

#include "xc.h"
#include "logic.h"
#include "buttons.h"
#include <string.h>
#include <time.h>

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
#include "display_driver.h"

#include "usb/usb.h"
#include "usb/usb_tasks.h"

/*#include "usb/usb_device_hid.h"
#include "usb/usb_tasks.h" */


//static FATFS drive;
//static FIL file;

#define MODIFY_BUFFER_SIZE 256

static uint8_t pin[PIN_SIZE];
static uint8_t pinerr = 0;

static uint8_t masterkey[16];
static bool keyset = false;

//static bool timeset = false;

//128bit AES key

typedef enum {
    IOR_UNCHANGED,
    IOR_UPDATED,    
    IOR_OK,
    IOR_ABORT,    
    IOR_CTX_SWITCH,
} HCTX_IO_RESULT;

const uint16_t highlight_color_tab[] = {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_CYAN,
    COLOR_MAGENTA,
    COLOR_YELLOW,
    COLOR_ORANGE
};

const TOKEN_CONFIG token_configs[] = {
    { NEW, TEXT_IOHEAD_NEW, TEXT_IOHEAD_NEW },
    { NAME, TEXT_IOHEAD_NAME, TEXT_VIEWHEAD_NAME  },
    { KEY1, TEXT_IOHEAD_KEY, TEXT_VIEWHEAD_KEY  },
    { KEY2, TEXT_IOHEAD_KEY2, TEXT_VIEWHEAD_KEY2 },
    { URL, TEXT_IOHEAD_URL, TEXT_VIEWHEAD_URL  },
    { INFO, TEXT_IOHEAD_INFO, TEXT_VIEWHEAD_INFO }        
};

DEVICE_OPTIONS device_options = {
    COLOR_YELLOW,
    COLOR_RED,
    100,
    USB_MODE_OFF    
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

            endEncryption( );
            
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
        case EDIT_ENTRY:
            return sizeof(CTX_EDIT_ENTRY);
        case MESSAGEBOX:
            return sizeof(CTX_MSG_BOX);
        case CHOOSEBOX:
            return sizeof(CTX_CHOOSE_BOX);           
        case ENTRY_DETAIL:
            return sizeof(CTX_ENTRY_DETAIL);
        case VIEW_TOKEN:
            return sizeof(CTX_VIEW_TOKEN);  
        case OPTIONS1:
            return sizeof(CTX_OPTIONS1);  
        case OPTIONS2:
            return sizeof(CTX_OPTIONS2);  
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

uint8_t getCharactersInLine(uint8_t* text, uint16_t coff, uint16_t maxchars ) {
    uint8_t c = 0;
    uint8_t lastspace = 0;
    uint8_t lastspacex = 0;
    uint8_t currx = 0;
    uint8_t xadv;
            
    while ( c < maxchars ) {
        xadv = gfxchars[ text[coff+c] < TOTAL_CHAR_COUNT ? text[coff+c] : CHAR_ENCODEERR ].xadv;        
        if ( currx + xadv > TEXTAREA_WIDTH ) {
            //need linebreak
            if ( lastspacex > MAX_TEXTAREA_SPACE_BREAK ) {
                return lastspace;
            } else {
                return c;
            }
        }
                
        if ( text[coff+c] == CHAR_LINEFEED && c > 0 ) return c;
        if ( text[coff+c] == CHAR_SPACE ) {
            lastspace = c + 1;
            lastspacex = currx +xadv;
        }

        currx += xadv;
        c++;
    }
    
    return maxchars;
}

void hctxIORecalcTavccoff( IO_CONTEXT *ioctx ) {
   uint16_t coffs = 0;
   uint16_t currline = 0;
   uint16_t count;
   while ( true ) {
       if ( currline == ioctx->tavcloff ) {
           ioctx->tavccoff = coffs;
           return;
       }
       count = getCharactersInLine(ioctx->text, coffs, ioctx->tlen - coffs );
       if ( count == 0 ) {
           ioctx->tavccoff = 0xffff;
           return;
       }
       coffs+= count;
       currline ++;               
   }
}  


uint8_t hctxIOWriteCharacter( IO_CONTEXT *ioctx, uint8_t cid, bool del ) {
    if ( ioctx->tlen >= MAX_TEXT_LEN && !del ) return 1;                
    
    if ( ioctx->tewp < ioctx->tavccoff ) return 2; //error, writepointer out of bounds
    
    uint16_t vpline = 0;
    uint16_t coffs = ioctx->tavccoff;
    
    while ( true ) {  
        uint8_t chklen = ioctx->tlen - coffs;
        if ( chklen == 0 ) break;
        
        uint8_t count = getCharactersInLine(ioctx->text, coffs, chklen );
        if ( coffs+count >= ioctx->tewp ) break;
        
        vpline ++;
        coffs += count;        
    }
            
    if ( del ) {
        if ( ioctx->tewp > 1 ) {
            memmove( ioctx->text+ioctx->tewp - 1, ioctx->text+ioctx->tewp, ioctx->tlen - ioctx->tewp );
        
            ioctx->tewp--;
            ioctx->tlen--;

            if ( vpline == 0 ) {
                //check cursor is still in viewport  
                hctxIORecalcTavccoff( ioctx );
                if ( ioctx->tavccoff >= ioctx->tewp ) {
                    if ( ioctx->tavcloff > 0 ) {
                        ioctx->tavcloff--;
                        hctxIORecalcTavccoff( ioctx );
                    }
                }            
            }     
        } else return 1;
        
    } else {       
        if ( ioctx->tewp < ioctx->tlen ) {
            memmove( ioctx->text+ioctx->tewp+1, ioctx->text+ioctx->tewp, ioctx->tlen - ioctx->tewp );
        }
        ioctx->text[ioctx->tewp] = cid;
        ioctx->tewp++;
        ioctx->tlen++;
        
        //if ( cid == CHAR_LINEFEED ) vpline++;
        
        if ( vpline == ( TEXTAREA_VIEWPORT_LINES - 1 ) ) {
            //check cursor is still in viewport            
            uint8_t newclen = getCharactersInLine(ioctx->text, coffs, ioctx->tlen - coffs );
            if ( ( coffs + newclen ) < ioctx->tewp ) {
                //out of viewport, move 1 line down
                ioctx->tavcloff++;
                ioctx->tavccoff += getCharactersInLine(ioctx->text, ioctx->tavccoff, ioctx->tlen - ioctx->tavccoff );
            }
        }        
    }

                    
    return 0;
}

void hctxIOGetCurrCursorPosition(IO_CONTEXT *ioctx, uint16_t *currline, uint8_t *posline, uint8_t *poslinex, uint8_t *currlinelen, uint16_t *currlineoffs, uint16_t *prevlineoffs ) {
    *currline = 0;
    uint16_t coff = 0;
    *prevlineoffs = 0;
    
    *poslinex = 0;
    
    while ( true ) {
        *currlinelen = getCharactersInLine( ioctx->text, coff, ioctx->tlen-coff ) ;
        if ( ioctx->tewp > coff && ioctx->tewp <= coff + *currlinelen ) {
            *currlineoffs = coff;
            *posline = ioctx->tewp - coff;
            for (uint16_t c = 0; c < *posline; c++) {
                *poslinex += gfxchars[ ioctx->text[coff+c] < TOTAL_CHAR_COUNT ? ioctx->text[coff+c] : CHAR_ENCODEERR ].xadv;                
            }
            return;
        } else {
            *prevlineoffs = coff;
        }
        
        (*currline)++;
        coff+= *currlinelen;
    }    
}

HCTX_IO_RESULT hctxIO( IO_CONTEXT *ioctx, APP_CONTEXT* ctx ) {
    
    if ( ioctx->selarea == 0 ) {
        //cursor on keyboard       
        if ( ioctx->mboxresult != MSGB_NO_RESULT ) {
            if ( ioctx->mboxresult == MSGB_YES ) {
                Keylayout *k = getCurrentKeyboardKey(ioctx);
                if ( k->fkt == KEYCOMMAND_OK ) {
                    memmove( ioctx->text, ioctx->text+1, ioctx->tlen ); //remove startlinefeed and add zero termination char to the end to save it properly on disk
                    ioctx->text[ioctx->tlen-1] = 0x00;
                    return IOR_OK;
                } else if ( k->fkt == KEYCOMMAND_ABORT ) {                
                    return IOR_ABORT;
                }                             
            }
            ioctx->mboxresult = MSGB_NO_RESULT;
        } else if ( isButtonPressed(BUTTON_A) ) {                                    
            Keylayout *k = getCurrentKeyboardKey(ioctx);

            if ( k->fkt == KEYCOMMAND_CHAR || k->fkt == KEYCOMMAND_SPACE || k->fkt == KEYCOMMAND_LF ) {
                uint8_t cid;
                switch (k->fkt) {
                    case KEYCOMMAND_CHAR: cid = k->id; break;
                    case KEYCOMMAND_SPACE: cid = CHAR_SPACE; break;
                    case KEYCOMMAND_LF: cid = CHAR_LINEFEED; break;                    
                }
                                                
                uint8_t res = hctxIOWriteCharacter( ioctx, cid, false );
                if ( res == 0 ) {
                    ioctx->datachanged = true;
                    ioctx->rinf = ONBUTTON_PRESS;
                }
            } else if ( k->fkt == KEYCOMMAND_BACKSPACE ) {
                uint8_t res = hctxIOWriteCharacter( ioctx, 0x00, true );
                if ( res == 0 ) {
                    ioctx->datachanged = true;
                    ioctx->rinf = ONBUTTON_PRESS;
                }                               
            } else if ( k->fkt == KEYCOMMAND_GEN ) {                
                Generatormaps *gm = (Generatormaps *)&generatormaps[ k->id ];
                uint8_t len = strlen( (char *)( textdata + gm->charset ) );
                uint8_t rng = ( 0xff / len ) * len;
                uint8_t rand[16];
                uint8_t randoff = 0xff;
                        
                                                
                for ( uint8_t ch = 0; ch< gm->len; ch++ ) {
                    while ( true ) {
                        if ( randoff >= 0x10 ) {
                            generateRND(rand);
                            randoff = 0;
                        }
                        
                        uint8_t cid = rand[randoff];
                        randoff ++;
                        if ( cid < rng ) {
                            cid = cid % len;
                            if ( hctxIOWriteCharacter( ioctx, textdata[ gm->charset + cid ], false ) == 0 ) {
                                ioctx->datachanged = true;
                            };
                            break;
                        }
                    }                                        
                }
                
                ioctx->rinf = ONBUTTON_PRESS;
            } else if ( k->fkt == KEYCOMMAND_ABORT ) {
                if ( ioctx->datachanged ) {
                    pushContext(ctx, MESSAGEBOX);
                    CTX_MSG_BOX *mctx;
                    mctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);
                    mctx->mtype = YES_NO;
                    mctx->res = MSGB_YES;
                    memcpy(mctx->msgtxt, (char*)(textdata + texte[TEXT_MSG_ABORT]), strlen( (char*)(textdata + texte[TEXT_MSG_ABORT]) ) + 1 );
                                        
                    return IOR_CTX_SWITCH;
                } else {
                    return IOR_ABORT;
                }
            } else if ( k->fkt == KEYCOMMAND_OK ) {
                if ( ioctx->datachanged && ioctx->type != NEW ) {
                    pushContext(ctx, MESSAGEBOX);
                    CTX_MSG_BOX *mctx;
                    mctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);
                    mctx->mtype = YES_NO;
                    mctx->res = MSGB_YES;
                    memcpy(mctx->msgtxt, (char*)(textdata + texte[TEXT_MSG_OVERWRITE]), strlen( (char*)(textdata + texte[TEXT_MSG_OVERWRITE]) ) + 1 );
                                        
                    return IOR_CTX_SWITCH;
                } else {                    
                    memmove( ioctx->text, ioctx->text+1, ioctx->tlen ); //remove startlinefeed and add zero termination char to the end to save it properly on disk
                    ioctx->text[ioctx->tlen-1] = 0x00;
                    return IOR_OK;
                }
            }
/*               
            } else if ( k->fkt == KEYCOMMAND_DEL ) {
                if ( ioctx->type != NEW ) {

                    pushContext(ctx, MESSAGEBOX);
                    CTX_MSG_BOX *mctx;
                    mctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);
                    mctx->mtype = YES_NO;
                    mctx->res = MSGB_YES;
                    
                    if ( ioctx->type == NAME ) {
                        memcpy(mctx->msgtxt, (char*)(textdata + texte[TEXT_MSG_DELETE_ENTRY]), strlen( (char*)(textdata + texte[TEXT_MSG_DELETE_ENTRY]) ) + 1 );
                    } else {
                        memcpy(mctx->msgtxt, (char*)(textdata + texte[TEXT_MSG_DELETE_TOKEN]), strlen( (char*)(textdata + texte[TEXT_MSG_DELETE_TOKEN]) ) + 1 );
                    }
                                        
                    return IOR_CTX_SWITCH;                                                                                
                }
            }*/
        } else if ( isButtonReleased(BUTTON_A) ) {
            ioctx->rinf = ONBUTTON_RELEASE;
        } else if ( isButtonDown(BUTTON_A) ) {
            //nothing to do, just block other buttons
        } else if ( isButtonPressed(BUTTON_UP) ||
             isButtonPressed(BUTTON_DOWN) ||
             isButtonPressed(BUTTON_LEFT) ||
             isButtonPressed(BUTTON_RIGHT) ) {
            
            if ( isButtonPressed(BUTTON_UP) && ioctx->kby == 0 ) {
                //select textarea
                ioctx->rinf = AREASWITCH;
                ioctx->oselarea = ioctx->selarea;
                ioctx->selarea = 1;             
                
                //prepare textarea info
                uint16_t currline;
                uint8_t posline;
                uint8_t poslinex;
                uint8_t currlinelen;
                uint16_t currlineoffs;
                uint16_t prevlineoffs;
                
                hctxIOGetCurrCursorPosition( ioctx, &currline, &posline, &poslinex, &currlinelen, &currlineoffs, &prevlineoffs );
                
                ioctx->tewpx = poslinex;                
            } else {
                ioctx->okby = ioctx->kby;
                ioctx->okbx = ioctx->kbx;                                
                ioctx->rinf = ANIMATION;

                Keylayout *k = getCurrentKeyboardKey(ioctx);
                                                
                if ( isButtonPressed(BUTTON_UP) && ioctx->kby > 0 ) ioctx->kby--;
                if ( isButtonPressed(BUTTON_DOWN) && ioctx->kby < 3 ) ioctx->kby++;
                if ( isButtonPressed(BUTTON_LEFT) ) {
                    int kx;
                    kx = ioctx->kbx;
                    
                    kx -= ( k->span_neg + 1 );
                    if ( kx < 0 ) kx = 9;
                    ioctx->kbx = kx;
                }
                if ( isButtonPressed(BUTTON_RIGHT) ) {
                    ioctx->kbx += ( k->span_pos + 1 );
                    if ( ioctx->kbx > 9 ) {
                        ioctx->kbx = 0;                        
                    }
                }                    
            }                                    
        } else if ( isButtonPressed( BUTTON_B) ) {
            ioctx->oselarea = ioctx->selarea;
            ioctx->selarea = 2; //open keymap
            ioctx->rinf = AREASWITCH;          
        }
   
    } else if ( ioctx->selarea == 1 ) {
        //cursor in textarea
        bool switchkb = false;
        if ( isButtonPressed(BUTTON_A) || isButtonPressed(BUTTON_B) ) {
            switchkb = true;
        } else if ( isButtonPressed(BUTTON_UP) ||
                    isButtonPressed(BUTTON_DOWN) ||
                    isButtonPressed(BUTTON_LEFT) ||
                    isButtonPressed(BUTTON_RIGHT) ) {
            
            uint16_t currline;
            uint8_t posline;
            uint8_t poslinex;
            uint8_t currlinelen;
            uint16_t currlineoffs;
            uint16_t prevlineoffs;

            hctxIOGetCurrCursorPosition( ioctx, &currline, &posline, &poslinex, &currlinelen, &currlineoffs, &prevlineoffs );

            if ( isButtonPressed(BUTTON_UP) ) {
                if ( currline > 0 ) {
                    poslinex = 0;
                    uint8_t distance = 0xff;                    
                    uint16_t newoffs = prevlineoffs;
                    for (uint16_t c = prevlineoffs; ; c++ ) {                        
                        uint8_t newdist;
                        newdist = poslinex >= ioctx->tewpx ? poslinex - ioctx->tewpx : ioctx->tewpx - poslinex;
                        if ( newdist <= distance ) {
                            distance = newdist;
                            newoffs = c; 
                        } else break;
                        if ( c >= currlineoffs ) break;
                        poslinex += gfxchars[ ioctx->text[c] < TOTAL_CHAR_COUNT ? ioctx->text[c] : CHAR_ENCODEERR ].xadv;
                    }
                                        
                    ioctx->tewp = newoffs;
                    if ( currline == ioctx->tavcloff ) {
                        ioctx->tavcloff--;
                        hctxIORecalcTavccoff( ioctx );
                    }
                }
            } else if ( isButtonPressed(BUTTON_DOWN) ) {
                if ( currlineoffs + currlinelen < ioctx->tlen ) {

                    poslinex = 0;
                    uint8_t distance = 0xff;                    
                    uint16_t newoffs = currlineoffs + currlinelen;
                    uint16_t maxoffs = newoffs + getCharactersInLine(ioctx->text, newoffs, ioctx->tlen - newoffs );                    
                    
                    for (uint16_t c = currlineoffs + currlinelen; ; c++ ) {                        
                        uint8_t newdist;
                        newdist = poslinex >= ioctx->tewpx ? poslinex - ioctx->tewpx : ioctx->tewpx - poslinex;
                        if ( newdist <= distance ) {
                            distance = newdist;
                            newoffs = c; 
                        } else break;
                        if ( c >= maxoffs ) break;
                        poslinex += gfxchars[ ioctx->text[c] < TOTAL_CHAR_COUNT ? ioctx->text[c] : CHAR_ENCODEERR ].xadv;
                    }
 
                    ioctx->tewp = newoffs;     
                    
                    if ( currline >= ( ioctx->tavcloff + ( TEXTAREA_VIEWPORT_LINES - 1 ) ) ) {
                        ioctx->tavcloff++;
                        hctxIORecalcTavccoff( ioctx );                        
                    }                                                                                                                     
                } else {
                    switchkb = true;
                }                                                               
            } else if ( isButtonPressed(BUTTON_LEFT) ) {
                Nop();

                if ( ioctx->tewp > 1 ) {
                    ioctx->tewp--;
                }
                                                
                if ( ioctx->tewp <= currlineoffs ) {                                                                            
                    currlineoffs = prevlineoffs;
                    if ( currline == ioctx->tavcloff && ioctx->tavcloff > 0) {                                                
                        ioctx->tavcloff--;
                        hctxIORecalcTavccoff( ioctx );                        
                    }
                }                   

                //recalculate xposition of cursor
                ioctx->tewpx = 0;
                uint8_t ccount = ioctx->tewp - currlineoffs;
                for (uint16_t c = 0; c < ccount; c++) {
                     ioctx->tewpx += gfxchars[ ioctx->text[currlineoffs + c] < TOTAL_CHAR_COUNT ? ioctx->text[currlineoffs + c] : CHAR_ENCODEERR ].xadv;
                }                                                                                               
 
            }  else if ( isButtonPressed(BUTTON_RIGHT) ) {
                if ( ioctx->tewp < ( ioctx->tlen ) ) {
                    ioctx->tewp++;
                }
                                                
                if ( ioctx->tewp > currlineoffs + currlinelen ) {                                                                            
                    currlineoffs = currlineoffs + currlinelen;
                    if ( currline == ioctx->tavcloff + ( TEXTAREA_VIEWPORT_LINES - 1 ) ) {                                                
                        ioctx->tavcloff++;
                        hctxIORecalcTavccoff( ioctx );                        
                    }
                }                   
                
                //recalculate xposition of cursor
                ioctx->tewpx = 0;
                uint8_t ccount = ioctx->tewp - currlineoffs;
                for (uint16_t c = 0; c < ccount; c++) {
                     ioctx->tewpx += gfxchars[ ioctx->text[currlineoffs + c] < TOTAL_CHAR_COUNT ? ioctx->text[currlineoffs + c] : CHAR_ENCODEERR ].xadv;
                }                                                                                               
            }   
            
            ioctx->rinf = ANIMATION;                    
        }
                        
        if ( switchkb ) {
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
            
            if ( keymaps[ioctx->kbmap].defx != 0xff ) {
                ioctx->kbx = keymaps[ioctx->kbmap].defx;
                ioctx->kby = keymaps[ioctx->kbmap].defy;
            }
        }        
    }
    
    if ( ioctx->rinf == ANIMATION || ioctx->rinf == AREASWITCH || ioctx->rinf == ONBUTTON_PRESS ) {
        return IOR_UPDATED;
    }
             
    return IOR_UNCHANGED;
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

FRESULT errAbortFileEntryHandling(FIL *file, TCHAR* unlinkpath ) {
    f_close(file);
    if ( unlinkpath != NULL ) {
        f_unlink( unlinkpath );
    }
    endEncryption( );
    return FR_DISK_ERR;    
}

FRESULT readEntryToken( TCHAR *path, TOKEN_TYPE type, uint8_t *text, uint16_t maxlen, uint16_t *tlen ) {
  
    FILINFO fno;
    FRESULT res;
    FIL file;
    UINT affected;
    uint8_t buffer[TOKEN_READ_MOD_BUFFERSIZE];
    int i, j;    
    uint8_t tokenpos;
    FSIZE_t readoffs;
    
    res = f_stat( path, &fno);
    if ( res == FR_OK && fno.fsize >= 48  && fno.fsize % 16 == 0 ) {
        res = f_open( &file, path, FA_READ | FA_WRITE );
        if ( res == FR_OK ) {
            res = f_read( &file, buffer, 48, &affected );
            if ( res != FR_OK || affected < 48 ) return errAbortFileEntryHandling( &file, NULL );
            
            //initialize encrypt/decrypt mode
            prepareAES128BitCBC();                    
            prepare128bitDecryption( buffer ); //Initialization Vector
                        
            // read entryheader and find current position of token or an alternative free space
            decrypt128bit( buffer+16, buffer+16 ); //random first block
            decrypt128bit( buffer+32, buffer+32 ); //header
            
            tokenpos = 0xff;
            for (i=32;i<48;i++) {
                if ( buffer[i] == type ) {
                    tokenpos = i-32;
                    break;
                }                    
            }
            
            *tlen = 0;
            if ( tokenpos == 0xff ) {
                text[0] = 0x00;
                *tlen = 1;
                f_close(&file);
                endEncryption();
                return FR_OK;
            }
                                   
            readoffs = tokenpos;
            readoffs = readoffs * TOKEN_BLOCK_SIZE + (48 - 16);
            f_lseek( &file, readoffs );
            
            res = f_read(&file, buffer, 16, &affected);
            if ( res != FR_OK || affected < 16 ) return errAbortFileEntryHandling( &file, NULL );
            
            prepare128bitDecryption( buffer );
            for ( i=0; i<TOKEN_BLOCK_SIZE / TOKEN_READ_MOD_BUFFERSIZE ; i++ ) {
                res = f_read(&file, buffer, TOKEN_READ_MOD_BUFFERSIZE, &affected);
                if ( res != FR_OK || affected < TOKEN_READ_MOD_BUFFERSIZE ) return errAbortFileEntryHandling( &file, NULL );
                
                for ( j=0; j<TOKEN_READ_MOD_BUFFERSIZE;j+=16 ) {
                    decrypt128bit( buffer+j, buffer+j );
                    uint8_t* boff = memchr(buffer+j, 0x00, 16);
                    if ( boff != NULL ) {
                        int len = ( boff - (buffer + j) ) + 1;
                        memcpy(text+*tlen, buffer+j, len );                        
                        *tlen += len;
                        f_close(&file);
                        endEncryption();
                        return FR_OK;
                    } else {
                        memcpy(text+*tlen, buffer+j, 16 );                        
                        *tlen += 16;                        
                    }
                    if ( *tlen >= maxlen ) {
                        text[maxlen-1] = 0x00;
                        f_close(&file);
                        endEncryption();
                        return FR_OK;                        
                    }
                }                
            }
            return errAbortFileEntryHandling( &file, NULL );
        }
    }
    return res;
}
            

FRESULT modifyEntry( TCHAR *path, uint8_t *text, uint16_t len, TOKEN_TYPE type ) {  
    uint8_t nonse[16];   
    FILINFO fno;
    FRESULT res;
    FIL file;
    UINT affected;
    uint8_t buffer[TOKEN_READ_MOD_BUFFERSIZE];
    int i, j, k;    
    uint8_t tokenpos;
    uint8_t freetokenpos;
    uint8_t lasttokenpos;
    uint16_t tokenoffs;
    FSIZE_t readoffs;
    
    generateRND( nonse ); //random nonse to add after nametag
    res = f_stat( path, &fno);
    if ( res == FR_OK && fno.fsize >= 32  && fno.fsize % 16 == 0 ) {
        res = f_open( &file, path, FA_READ | FA_WRITE );
        if ( res == FR_OK ) {
            res = f_read( &file, buffer, 48, &affected );
            if ( res != FR_OK || affected < 48 ) return errAbortFileEntryHandling( &file, NULL );
            
            //initialize encrypt/decrypt mode
            prepareAES128BitCBC();                    
            prepare128bitEncryption( buffer ); //Initialization Vector
            switch128BitDecryptEncrypt();
            prepare128bitDecryption( buffer ); //Initialization Vector
                        
            // read entryheader and find current position of token or an alternative free space
            decrypt128bit( buffer+16, buffer+16 ); //random first block
            decrypt128bit( buffer+32, buffer+32 ); //header
            
            tokenpos = 0xff;
            freetokenpos = 0xff;
            lasttokenpos = 0xff;
            for (i=32;i<48;i++) {
                if ( buffer[i] == type ) tokenpos = i-32;                
                if ( buffer[i] == NEW && freetokenpos == 0xff ) freetokenpos = i-32;
                if ( buffer[i] != NEW && ( lasttokenpos == 0xff || (i-32) > lasttokenpos ) ) lasttokenpos = i-32;
            }
            if ( tokenpos == 0xff ) {
                if ( len == 0 ) {
                    //no changes, token dont exist
                    f_close(&file);
                    endEncryption();
                    return FR_OK;
                }
                if ( freetokenpos == 0xff ) return errAbortFileEntryHandling( &file, path );
                tokenpos = freetokenpos;
                buffer[ freetokenpos + 32] = type;
            }
                                                            
            //encrypt the header
            switch128BitDecryptEncrypt();
            encrypt128bit( buffer+16, buffer+16 ); //random first block
            encrypt128bit( buffer+32, buffer+32 ); //header
            
            f_lseek( &file, 0 );
            res = f_write( &file, buffer, 48, &affected );
            if ( res != FR_OK || affected < 48 ) return errAbortFileEntryHandling( &file, path );
            f_sync(&file);
                        
            switch128BitDecryptEncrypt();
            readoffs = 48;
            //advance to the token to overwrite
            for ( int mode =0;mode < 3; mode++) {
                if ( mode == 0) {
                    tokenoffs = ( TOKEN_BLOCK_SIZE / TOKEN_READ_MOD_BUFFERSIZE ) * tokenpos;
                } else if ( mode == 1 ) {
                    tokenoffs = TOKEN_BLOCK_SIZE / TOKEN_READ_MOD_BUFFERSIZE;
                    if ( lasttokenpos == 0xff || lasttokenpos < tokenpos ) {
                        //expand file and get back to current position
                        readoffs += TOKEN_BLOCK_SIZE;
                        f_lseek( &file, readoffs );
                        readoffs -= TOKEN_BLOCK_SIZE;
                        f_lseek( &file, readoffs );
                        lasttokenpos += 1;
                    }
                } else if ( mode == 2 ) {
                    tokenoffs = ( lasttokenpos - tokenpos ) * ( TOKEN_BLOCK_SIZE / TOKEN_READ_MOD_BUFFERSIZE );
                }
                
                for ( i=0; i<tokenoffs; i++ ) {
                    res = f_read( &file, buffer, TOKEN_READ_MOD_BUFFERSIZE, &affected );
                    if ( res != FR_OK || affected < TOKEN_READ_MOD_BUFFERSIZE ) return errAbortFileEntryHandling( &file, path );

                    for (k=0;k<sizeof(buffer);k+=16) decrypt128bit( buffer+k, buffer+k );
                    switch128BitDecryptEncrypt();                                

                    if ( mode == 1 ) {
                        //modify tokendata
                        if ( i*TOKEN_READ_MOD_BUFFERSIZE > len ) {
                            for (k=0;k<TOKEN_READ_MOD_BUFFERSIZE;k+=16) memcpy(buffer+k, nonse, 16);
                        } else {
                            memcpy(buffer, text+i*TOKEN_READ_MOD_BUFFERSIZE, len - i*TOKEN_READ_MOD_BUFFERSIZE);
                                                        
                            j = len/16*16;
                            if ( len %16 != 0 ) j+= 16;
                                                       
                            for (k=j;k<TOKEN_READ_MOD_BUFFERSIZE;k+=16) memcpy(buffer+k, nonse, 16);
                        }
                    }
                    
                    for (k=0;k<TOKEN_READ_MOD_BUFFERSIZE;k+=16) encrypt128bit( buffer+k, buffer+k );
                    switch128BitDecryptEncrypt();

                    f_lseek(&file, readoffs );
                    readoffs+= TOKEN_READ_MOD_BUFFERSIZE;

                    res = f_write( &file, buffer, TOKEN_READ_MOD_BUFFERSIZE, &affected );
                    if ( res != FR_OK || affected < TOKEN_READ_MOD_BUFFERSIZE ) return errAbortFileEntryHandling( &file, path );
                }      
            }
                        
            readoffs = lasttokenpos+1;
            readoffs = readoffs * TOKEN_BLOCK_SIZE + 48;            
            res = f_lseek( &file, readoffs );
            if ( res != FR_OK ) return errAbortFileEntryHandling( &file, path );
            
            f_truncate( &file );            
            f_close(&file);
            endEncryption();                       
        }       
    }

    return res;
}


FRESULT createNewEntry( TCHAR *path, uint8_t *text, uint16_t len ) {    
    uint8_t cipher[16];
    uint8_t plain[16];   
    FILINFO fno;
    FRESULT res;
    FIL file;
    UINT written;
    
    memcpy(path, &"KS/", 3);
    while ( true ) {
        generateRND( cipher );
                
        for ( int i=0; i<12; i++ ) {
            char c = cipher[i] % 16;
            if ( c < 10 ) { 
                path[3+i] = '0' + c;
            } else {
                path[3+i] = 'a' + c;
            }                       
        }             
        path[11] = '.';
        path[16] = 0x00;
        
        res = f_stat( path, &fno);
        if ( res == FR_OK || res == FR_NO_FILE ) {
            if ( res == FR_NO_FILE ) {
                res = f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS );
                if ( res == FR_OK ) {                    
                    generateRND( cipher ); //Initialization Vector
                    res = f_write(&file, cipher, 16, &written );
                    if ( res != FR_OK || written < 16 ) {
                        return errAbortFileEntryHandling(&file, path);
                    }

                    generateRND( plain ); //Random first encryption block
                    
                    prepareAES128BitCBC();                    
                    prepare128bitEncryption( cipher );                                        

                    encrypt128bit( plain, cipher );                            
                    res = f_write(&file, cipher, 16, &written );
                    if ( res != FR_OK || written < 16 ) {
                        return errAbortFileEntryHandling(&file, path);
                    }
                    
                    memset( plain, NEW , 16 );                                        
                    encrypt128bit( plain, cipher );                            
                    res = f_write(&file, cipher, 16, &written );
                    if ( res != FR_OK || written < 16 ) {
                        return errAbortFileEntryHandling(&file, path);
                    }                       
                    
                    res = f_close(&file);
                    if ( res != FR_OK || written < 16 ) {
                        return errAbortFileEntryHandling(&file, path);
                    }
                    endEncryption( ); 
                    break;
                }
            }                        
        } else {
            break;
        }
    }
    return res;
}


FRESULT deleteEntry(  TCHAR *path ) {
    FIL file;
    FILINFO fno;
    FRESULT res;     
    uint8_t shred[128];
    uint8_t blocks = 0;
    uint8_t lastblock = 0;
    UINT written;
    
    res = f_stat(path, &fno );
    if ( res == FR_OK ) {
        uint16_t size = fno.fsize;
        if ( size > 0 ) {            
            res = f_open(&file, path, FA_WRITE );
            if ( res == FR_OK ) {
                //shred file content
                memset(shred, 0xFF, 128);
                blocks = size / 128;
                lastblock = size % 128;        
                
                for ( uint8_t bc = 0; bc < blocks; bc++) {
                    f_write(&file, shred, 128, &written);
                }
                if ( lastblock > 0 ) {
                    f_write(&file, shred, lastblock, &written);
                }
                f_close(&file);
            }
            res = f_unlink( path );
        }
    }
    return res;    
}

FRESULT loadEntryDetail( APP_CONTEXT* ctx ) {
    
    uint16_t tlen;
    CTX_ENTRY_DETAIL *sctx;
    sctx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);  
         
    sctx->selected = NAME;
  
    FRESULT res;  
    res = readEntryToken( sctx->path, NAME, sctx->name, 80, &tlen );    
    if ( res != FR_OK ) return res;   
    
    readEntryToken( sctx->path, KEY1, sctx->key1, 32, &tlen );    
    memset( sctx->key1, PASSWORD_REPLACE_CHAR, tlen-1 );

    readEntryToken( sctx->path, KEY2, sctx->key2, 32, &tlen );    
    memset( sctx->key2, PASSWORD_REPLACE_CHAR, tlen-1 );
    
    readEntryToken( sctx->path, URL, sctx->url, 80, &tlen );    

    readEntryToken( sctx->path, INFO, sctx->info, 80, &tlen );    
    
    return FR_OK;
    
}

FRESULT hctxEntryOverviewReloadList( APP_CONTEXT* ctx ) {
    CTX_ENTRY_OVERVIEW *sctx;
    sctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);

    FRESULT res;
    DIR dir;
    static FILINFO fno;
    
    res = f_opendir(&dir, "KS");                       
    if (res == FR_OK) {
        int32_t start = sctx->cursor;
        start = start - MAX_OVERVIEW_ENTRY_COUNT / 2;
        if ( start < 0 ) start = 0;            
        uint16_t end = sctx->cursor + ( MAX_OVERVIEW_ENTRY_COUNT / 2 ) - 1;
        if ( end >= sctx->entrycount ) {
            end = sctx->entrycount;
        }        
        sctx->entrycenter = sctx->cursor;
        
        uint16_t curr = 0;
        while ( true ) {
            res = f_readdir(&dir, &fno);                                                                      
            if ( curr == start ) sctx->bufferstart = curr;
            if ( curr == end ) {
                sctx->bufferlen = curr - sctx->bufferstart;
                break;
            }
            if (res != FR_OK || fno.fname[0] == 0) return FR_DISK_ERR;
                        
            if ( curr >= start && curr < end ) {                
                memcpy( sctx->entries[ curr - sctx->bufferstart ].path , &fno.fname, strlen( (char*)&fno.fname ) + 1 );
                sctx->entries[ curr - sctx->bufferstart ].loaded = false;
            }            
            curr++;
        }
        f_closedir(&dir);
    }  
    return FR_OK;
}

void hctxEntryOverviewLoadNames( APP_CONTEXT* ctx ) {
    CTX_ENTRY_OVERVIEW *sctx;
    sctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);    
    
    FRESULT res;
    
    //loadnext entryname
    bool direction = true;
    uint8_t offs = 0;        
    while ( true ) {
        int32_t pos = sctx->cursor - sctx->bufferstart;
        if ( direction ) pos += offs;
        else pos -= offs;

        if ( pos >= 0 && pos < sctx->bufferlen ) {
            if ( !sctx->entries[pos].loaded ) {
                uint16_t len;
                TCHAR path[16];

                memcpy( path, &"KS/", 3 );                    
                memcpy( path+3, sctx->entries[pos].path, strlen( sctx->entries[pos].path ) + 1 );                    

                res = readEntryToken( path, NAME, sctx->entries[pos].name, 47, &len );
                if ( res != FR_OK ) {
                    setContext( ctx, ERROR_SD_FAILURE  );
                } else {
                    sctx->entries[pos].loaded = true;
                    ctx->rinf = LOADENTRY;
                }
                if ( offs >= 5) break;
            }
        }
        if ( direction ) offs++;
        direction = !direction;
        if ( offs > MAX_OVERVIEW_ENTRY_COUNT / 2 ) {
            break; //everything loaded
        }            
    }    
}


void hctxEntryOverview(APP_CONTEXT* ctx) {
    CTX_ENTRY_OVERVIEW *sctx;
    sctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);
 
    FRESULT res;
    DIR dir;
    static FILINFO fno;
    
//    uint16_t cursor = sctx->cursor;
    
    if ( !sctx->initialized ) {
        sctx->initialized = true;
        
        sctx->entrycount = 0;
        res = f_opendir(&dir, "KS");                       
        if (res == FR_OK) {
            while ( true ) {
                res = f_readdir(&dir, &fno);                   
                if (res != FR_OK || fno.fname[0] == 0) break;  
                sctx->entrycount++;
            }
            f_closedir(&dir);
        }
        if ( sctx->cursor >= sctx->entrycount ) sctx->cursor = sctx->entrycount > 0 ? sctx->entrycount - 1 : 0;
        
        hctxEntryOverviewReloadList( ctx );
        hctxEntryOverviewLoadNames( ctx );       
        ctx->rinf = ANIMATION;
    } else if ( isButtonPressed(BUTTON_A) ) {
/*        if ( replaceContext(ctx, EDIT_ENTRY ) ) {
            CTX_EDIT_ENTRY *ectx;
            ectx = (CTX_EDIT_ENTRY*) (ctx->ctxbuffer + ctx->ctxptr);
            ectx->io.type = NEW;
            ectx->io.rinf = REFRESH;
            ectx->io.kbmap = DEFAULT_KEYMAP;
            ectx->io.text[0] = CHAR_LINEFEED;
            ectx->io.tlen = 1;
            ectx->io.tewp = 1;
            ectx->overviewcursor = cursor;
            ctx->rinf = IO_UPDATE;
        } */
        
        if ( sctx->cursor < sctx->entrycount ) {
            TCHAR path[16];
            uint16_t cursor = sctx->cursor;
                        
            memcpy( path, &"KS/", 3 );                    
            memcpy( path+3, sctx->entries[cursor].path, strlen( sctx->entries[cursor].path ) + 1 );
                        
            replaceContext(ctx, ENTRY_DETAIL );

            CTX_ENTRY_DETAIL *edctx;
            edctx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);
                       
            memcpy( edctx->path, path, 16 );
            if ( loadEntryDetail( ctx ) != FR_OK ) {
                setContext( ctx, ERROR_SD_FAILURE  );
            } else {
                edctx->selected = NAME;
                edctx->overviewcursor = cursor;
            }
        }         
        
        
    } else if ( isButtonPressed(BUTTON_B) ) {        
        bool candelete = false;
        if ( sctx->entrycount > 0 ) {
            candelete = true;
        }
  
        pushContext(ctx, CHOOSEBOX );
        
        CTX_CHOOSE_BOX *cbctx;
        cbctx = (CTX_CHOOSE_BOX*) (ctx->ctxbuffer + ctx->ctxptr);        
        
        if ( candelete ) {
            cbctx->options = 3;
            cbctx->textid[0] = TEXT_CHB_CREATE_ENTRY;
            cbctx->selectid[0] = CHBX_CREATE_ENTRY;
            cbctx->textid[1] = TEXT_CHB_DELETE_ENTRY;
            cbctx->selectid[1] = CHBX_DELETE_ENTRY;
            cbctx->textid[2] = TEXT_CHB_CONFIG;
            cbctx->selectid[2] = CHBX_CONFIG;            
        } else {
            cbctx->options = 2;
            cbctx->textid[0] = TEXT_CHB_CREATE_ENTRY;
            cbctx->selectid[0] = CHBX_CREATE_ENTRY;
            cbctx->textid[2] = TEXT_CHB_CONFIG;
            cbctx->selectid[2] = CHBX_CONFIG;            
        }
                              
    } else if ( isButtonPressed(BUTTON_RIGHT) ) {
/*        
        if ( sctx->cursor < sctx->entrycount ) {
            TCHAR path[16];
            uint16_t cursor = sctx->cursor;
                        
            memcpy( path, &"KS/", 3 );                    
            memcpy( path+3, sctx->entries[cursor].path, strlen( sctx->entries[cursor].path ) + 1 );
                        
            replaceContext(ctx, ENTRY_DETAIL );

            CTX_ENTRY_DETAIL *edctx;
            edctx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);
                       
            memcpy( edctx->path, path, 16 );
            if ( loadEntryDetail( ctx ) != FR_OK ) {
                setContext( ctx, ERROR_SD_FAILURE  );
            } else {
                edctx->selected = NAME;
                edctx->overviewcursor = cursor;
            }
        }                               
*/ 
    } else if ( isButtonPressed(BUTTON_UP) || isButtonPressed(BUTTON_DOWN) ) {
        if ( isButtonPressed(BUTTON_UP) && sctx->cursor > 0 ) sctx->cursor--;
        if ( isButtonPressed(BUTTON_DOWN) && sctx->cursor < ( sctx->entrycount - 1 ) ) sctx->cursor++;
        
        uint16_t diff;
        diff = sctx->cursor < sctx->entrycenter ? sctx->entrycenter - sctx->cursor : sctx->cursor - sctx->entrycenter;
        if ( diff > OVERVIEW_RELOAD_OFFS ) {
            hctxEntryOverviewReloadList( ctx );
            hctxEntryOverviewLoadNames( ctx );
        }
        ctx->rinf = ANIMATION;
    } else {
        //loadnext entryname
        hctxEntryOverviewLoadNames( ctx );             
    }            
}

void hctxEntryDetail(APP_CONTEXT* ctx) {
    CTX_ENTRY_DETAIL *sctx;
    sctx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);
    
    TCHAR path[16];
    TOKEN_TYPE currtoken;
    FRESULT res;
    
    if ( isButtonPressed(BUTTON_UP) ) {
        if ( sctx->selected > NAME ) {
            sctx->oselected = sctx->selected;
            sctx->selected --;
            ctx->rinf = ANIMATION;
        }        
    } else if ( isButtonPressed(BUTTON_DOWN) ) {
        if ( sctx->selected < (CHECK-1) ) {
            sctx->oselected = sctx->selected;
            sctx->selected ++;
            ctx->rinf = ANIMATION;
        }                
    } else if ( isButtonPressed(BUTTON_RIGHT) ) {
        //view token
        pushContext(ctx, VIEW_TOKEN);
        
        CTX_VIEW_TOKEN *vtctx;
        vtctx = (CTX_VIEW_TOKEN*) (ctx->ctxbuffer + ctx->ctxptr); 

        vtctx->type = sctx->selected;
                             
        res = readEntryToken( sctx->path, sctx->selected, vtctx->text, MAX_TEXT_LEN, &vtctx->tlen ); 
        memmove(vtctx->text+1, vtctx->text, vtctx->tlen);
        vtctx->text[0] = CHAR_LINEFEED;
        if ( res != FR_OK ) { 
            setContext( ctx, ERROR_SD_FAILURE  );
        } else {            
            uint16_t offs, len;        
            vtctx->lines = 1;
            offs = 0;
            while (true) {
                len = getCharactersInLine(vtctx->text, offs, vtctx->tlen-offs );
                if ( len < vtctx->tlen-offs ) {
                    vtctx->lines++;                    
                } else break;
                offs += len;
            }
        }
                
    } else if ( isButtonPressed(BUTTON_A) ) {
        //edit token       
        if ( sctx->selected != NEW ) {
            memcpy( path, sctx->path, 16 );
            currtoken = sctx->selected;
            
            replaceContext(ctx, EDIT_ENTRY ); 

            CTX_EDIT_ENTRY *edctx;
            edctx = (CTX_EDIT_ENTRY*) (ctx->ctxbuffer + ctx->ctxptr); 

            memcpy( edctx->path, path, 16 );                        
            edctx->io.type = currtoken;
            edctx->io.text[0] = CHAR_LINEFEED;            
            ctx->rinf = IO_UPDATE;
            
            res = readEntryToken( edctx->path, edctx->io.type, edctx->io.text+1, MAX_TEXT_LEN, &edctx->io.tlen );
            if ( res == FR_OK ) {                   
                edctx->io.tewp = edctx->io.tlen;
                edctx->io.kbmap = DEFAULT_KEYMAP;
                edctx->io.rinf = REFRESH;
                
                
                uint16_t currline, currlineoffs, prevlineoffs;
                uint8_t posline, poslinex, currlinelen;
                
                hctxIOGetCurrCursorPosition(&edctx->io, &currline, &posline, &poslinex, &currlinelen, &currlineoffs, &prevlineoffs );
                if ( currline > 0 ) {
                    uint8_t loff = currline;
                    if ( loff >= ( TEXTAREA_VIEWPORT_LINES - 1 ) ) loff = TEXTAREA_VIEWPORT_LINES - 1;
                    for (int i=0;i<loff;i++) {
                        edctx->io.tewp -= currlinelen;                        
                        hctxIOGetCurrCursorPosition(&edctx->io, &currline, &posline, &poslinex, &currlinelen, &currlineoffs, &prevlineoffs );
                    }                 
                    edctx->io.tewp = edctx->io.tlen;
                }
                
                edctx->io.tavcloff = currline;
                hctxIORecalcTavccoff( &edctx->io );

            } else {
                setContext( ctx, ERROR_SD_FAILURE  );
            }                        
        }                       
    } else if ( isButtonPressed(BUTTON_B) ) {
        //keyboard write token        
    } else if ( isButtonPressed(BUTTON_LEFT) ) {
        uint16_t cursor = sctx->overviewcursor;
        
        setContext( ctx, ENTRY_OVERVIEW );
        CTX_ENTRY_OVERVIEW *ovctx;
        ovctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr); 
        ovctx->cursor = cursor;
    }
}

void hctxViewToken(APP_CONTEXT* ctx) {
    CTX_VIEW_TOKEN *sctx;
    sctx = (CTX_VIEW_TOKEN*) (ctx->ctxbuffer + ctx->ctxptr);
            
    if ( isButtonPressed(BUTTON_DOWN) || isButtonPressed(BUTTON_UP) ) {         
        if ( isButtonPressed(BUTTON_UP) && sctx->currline > 0 ) sctx->currline--;
        int32_t lines = sctx->lines;
        lines -= TEXTVIEWER_LINES;
        if ( lines < 0 ) lines = 0;        
        if ( isButtonPressed(BUTTON_DOWN) && sctx->currline < lines ) sctx->currline++;   
        ctx->rinf = ANIMATION;
    } else if ( isButtonPressed(BUTTON_LEFT) ) {
        removeContext(ctx);
    }
}


void hctxEditEntry(APP_CONTEXT* ctx) {
    TCHAR path[16];
    TOKEN_TYPE currtoken;
    uint16_t cursor;
    
    CTX_EDIT_ENTRY *sctx;
    sctx = (CTX_EDIT_ENTRY*) (ctx->ctxbuffer + ctx->ctxptr);                       
            
    HCTX_IO_RESULT res = hctxIO( &sctx->io, ctx );    
    if ( res == IOR_CTX_SWITCH ) {
        
    } else if ( res == IOR_UNCHANGED ) {
        ctx->rinf = UNCHANGED;
    } else if ( res == IOR_UPDATED ) {
        ctx->rinf = IO_UPDATE;        
    } else if ( res == IOR_OK ) {
        //Input done
        if ( sctx->io.type == NEW ) {
            //create an empty entry
            FRESULT fres = createNewEntry(sctx->path, (uint8_t *)(sctx->io.text), sctx->io.tlen );
            if ( fres != FR_OK ) {
                setContext( ctx, ERROR_SD_FAILURE  );
            }
            sctx->io.type = NAME; //set the token to name and modify it after
        }        
        memcpy( path, sctx->path, 16 );
        
        //write the the tokendata
        FRESULT fres = modifyEntry(sctx->path, (uint8_t *)(sctx->io.text), sctx->io.tlen, sctx->io.type );  
        if ( fres != FR_OK ) {
            setContext( ctx, ERROR_SD_FAILURE  );        
        }
        
        if ( ctx->ctxtype != ERROR_SD_FAILURE ) {
            memcpy( path, sctx->path, 16 );                         
            currtoken = sctx->io.type;
            cursor = sctx->overviewcursor;
            
            replaceContext(ctx, ENTRY_DETAIL );
            
            CTX_ENTRY_DETAIL *edctx;
            edctx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);                       
            
            memcpy( edctx->path, path, 16 );
            
            if ( loadEntryDetail( ctx ) != FR_OK ) {
                setContext( ctx, ERROR_SD_FAILURE  );
            } else {
                if ( currtoken == NEW ) currtoken = NAME;
                edctx->selected = currtoken;
                edctx->overviewcursor = cursor;
            }
        }
    } else if ( res == IOR_ABORT ) {
        //Input aborted
        if ( sctx->io.type == NEW ) {
            setContext(ctx, ENTRY_OVERVIEW );
        } else {                       
            memcpy( path, sctx->path, 16 );                         
            currtoken = sctx->io.type;
            cursor = sctx->overviewcursor;

            replaceContext(ctx, ENTRY_DETAIL );
            
            CTX_ENTRY_DETAIL *edctx;
            edctx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);                       
            
            memcpy( edctx->path, path, 16 );
            
            if ( loadEntryDetail( ctx ) != FR_OK ) {
                setContext( ctx, ERROR_SD_FAILURE  );
            } else {
                edctx->selected = currtoken;
                edctx->overviewcursor = cursor;
            }
        }
    }
/*    } else if ( res == IOR_DELETE ) {
        //Delete entry
        memcpy( path, sctx->path, 16 ); 
        cursor = sctx->overviewcursor;
        
        setContext(ctx, ENTRY_OVERVIEW );
        if ( deleteEntry( path ) != FR_OK ) {
            setContext( ctx, ERROR_SD_FAILURE  );
        } else {
            CTX_ENTRY_OVERVIEW *edctx;
            edctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);                                   
            edctx->cursor = cursor;
        }
    }*/
}

void hctxMessagebox(APP_CONTEXT* ctx) {
    CTX_MSG_BOX *sctx;
    sctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);
    
    if ( isButtonPressed(BUTTON_LEFT) ) {
        sctx->res = MSGB_YES;
        ctx->rinf = ANIMATION;
    } else if ( isButtonPressed(BUTTON_RIGHT) ) {
        sctx->res = MSGB_NO;
        ctx->rinf = ANIMATION;
    } else if ( isButtonPressed(BUTTON_A) || isButtonPressed(BUTTON_B) ) {
        if ( sctx->res != MSGB_NO_RESULT ) {
            MSGB_RESULT result = sctx->res;
            removeContext(ctx);
            if ( ctx->ctxtype == EDIT_ENTRY ) {
                CTX_EDIT_ENTRY *prevctx;
                prevctx = (CTX_EDIT_ENTRY*) (ctx->ctxbuffer + ctx->ctxptr);                
                prevctx->io.mboxresult = result;
                prevctx->io.rinf = REFRESH;
                ctx->rinf = IO_UPDATE;
            }
        }        
    }   
}

void hctxChoosebox(APP_CONTEXT* ctx) {
    CTX_CHOOSE_BOX *sctx;
    sctx = (CTX_CHOOSE_BOX*) (ctx->ctxbuffer + ctx->ctxptr);
    
    if ( isButtonPressed(BUTTON_UP) ) {
        if ( sctx->selected > 0 ) {
            sctx->oselected = sctx->selected;
            sctx->selected--;
            ctx->rinf = ANIMATION;
        }                
    } else if ( isButtonPressed(BUTTON_DOWN) ) {
        if ( sctx->selected < ( sctx->options - 1 ) ) {
            sctx->oselected = sctx->selected;
            sctx->selected++;
            ctx->rinf = ANIMATION;
        }  
    } else if ( isButtonPressed(BUTTON_A) ) {
        CHBX_SELECTIONS sel = sctx->selectid[ sctx->selected ];
                
        if ( sel == CHBX_CREATE_ENTRY ) {
            removeContext(ctx);
            
            CTX_ENTRY_OVERVIEW *ovctx;
            ovctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);            
            uint16_t cursor = ovctx->cursor;
            
            if ( replaceContext(ctx, EDIT_ENTRY ) ) {
                CTX_EDIT_ENTRY *ectx;
                ectx = (CTX_EDIT_ENTRY*) (ctx->ctxbuffer + ctx->ctxptr);
                ectx->io.type = NEW;
                ectx->io.rinf = REFRESH;
                ectx->io.kbmap = DEFAULT_KEYMAP;
                ectx->io.text[0] = CHAR_LINEFEED;
                ectx->io.tlen = 1;
                ectx->io.tewp = 1;
                ectx->overviewcursor = cursor;                
                ctx->rinf = IO_UPDATE;
            }                     
        } else if ( sel == CHBX_DELETE_ENTRY ) {
            removeContext(ctx);
            
            CTX_ENTRY_OVERVIEW *ovctx;
            ovctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);
                                        
            //Delete entry
            
            TCHAR path[16];
            memcpy(path, &"KS/", 3);
            memcpy(path+3, ovctx->entries[ovctx->cursor].path, strlen(ovctx->entries[ovctx->cursor].path) + 1 );
            
            if ( deleteEntry( path ) != FR_OK ) {
                setContext( ctx, ERROR_SD_FAILURE  );
            } else {
                ovctx->initialized = false;
            }
                        
        } else if ( sel == CHBX_CONFIG ) {
            setContext( ctx, OPTIONS1 );            
        }
                    
    } else if ( isButtonPressed(BUTTON_B) ) {
        removeContext(ctx);
    }
    
}

uint8_t hctxGetColor() {
    
    for (uint8_t i= 0;i<sizeof(highlight_color_tab)/sizeof(uint16_t);i++ ) {
        if ( highlight_color_tab[i] == device_options.highlight_color1 ) return i;              
    }    
    return 0;    
}

void hctxSetColor(uint8_t c ) {    
    device_options.highlight_color1  = highlight_color_tab[c];
    device_options.highlight_color2  = device_options.highlight_color1;   
}


void hctxOptions1(APP_CONTEXT* ctx) {
    CTX_OPTIONS1 *sctx;
    sctx = (CTX_OPTIONS1*) (ctx->ctxbuffer + ctx->ctxptr);    
          
    if ( isButtonPressed(BUTTON_DOWN) ) {
        sctx->oselected = sctx->selected;
        if ( sctx->selected < 2 ) sctx->selected ++;
        ctx->rinf = ANIMATION;
    } else if ( isButtonPressed(BUTTON_UP) ) {
        sctx->oselected = sctx->selected;
        if ( sctx->selected > 0 ) sctx->selected --;                
        ctx->rinf = ANIMATION;
    } else if ( isButtonPressed( BUTTON_LEFT ) ) {
        if ( sctx->selected == 0 ) {
            device_options.umode = device_options.umode == USB_MODE_OFF ? USB_MODE_KEYBOARD : USB_MODE_OFF;
        } else if ( sctx->selected == 1 ) {
            device_options.brightness = device_options.brightness > 10 ? device_options.brightness - 10 : device_options.brightness;
            dispSetBrightness( device_options.brightness );
        } else if ( sctx->selected == 2 ) {
            uint8_t c = hctxGetColor();
            if ( c > 0 ) { 
                hctxSetColor(c-1); 
            } else { 
                hctxSetColor(sizeof(highlight_color_tab)/sizeof(uint16_t) - 1); 
            };            
        }
        ctx->rinf = ANIMATION;
    } else if ( isButtonPressed( BUTTON_RIGHT ) ) {
        if ( sctx->selected == 0 ) {
            device_options.umode = device_options.umode == USB_MODE_OFF ? USB_MODE_KEYBOARD : USB_MODE_OFF;
        } else if ( sctx->selected == 1 ) {
            device_options.brightness = device_options.brightness < 100 ? device_options.brightness + 10 : device_options.brightness;
            dispSetBrightness( device_options.brightness );
        } else if ( sctx->selected == 2 ) {
            uint8_t c = hctxGetColor();
            if ( c < sizeof(highlight_color_tab)/sizeof(uint16_t) - 1 ) { 
                hctxSetColor(c + 1);
            } else {
                hctxSetColor(0);
            }
        }
        ctx->rinf = ANIMATION;
    } else if ( isButtonPressed( BUTTON_B ) ) {
        replaceContext(ctx, OPTIONS2 );        
    }
}

int getMaxDaysOfMonth(int year, int month) {
    switch ( month ) {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return 31;
        case 2:
            if ( year % 4 == 0 ) return 29; //Schaltjahr ohne jahrhundertregelung
            return 28;
        default:
            return 30;
    }
}


void hctxOptions2(APP_CONTEXT* ctx) {
    CTX_OPTIONS2 *sctx;
    sctx = (CTX_OPTIONS2*) (ctx->ctxbuffer + ctx->ctxptr);    

    ctx->rinf = ANIMATION;
    if ( sctx->selected >= 1 && sctx->selected <= 3 ) {
        RTCC_TimeSet( &sctx->time );
    } else {
        RTCC_TimeGet( &sctx->time );
    }
        
    sctx->oselected = 0xff;    
    if ( isButtonPressed(BUTTON_DOWN) ) {        
        sctx->oselected = sctx->selected;
        if ( sctx->selected == 0 ) {
            sctx->selected = 4;
        } else if ( sctx->selected == 1 ) {
            sctx->time.tm_hour --;
            if ( sctx->time.tm_hour < 0 ) sctx->time.tm_hour = 23;
        } else if ( sctx->selected == 2 ) {
            sctx->time.tm_min --;
            if ( sctx->time.tm_min < 0 ) sctx->time.tm_min = 59;                        
        } else if ( sctx->selected == 3 ) {            
            sctx->time.tm_sec --;
            if ( sctx->time.tm_sec < 0 ) sctx->time.tm_sec = 59;
        } else if ( sctx->selected == 5 ) {
            sctx->time.tm_mday --;
            if ( sctx->time.tm_mday < 1 ) sctx->time.tm_mday = getMaxDaysOfMonth( sctx->time.tm_year, sctx->time.tm_mon );
        } else if ( sctx->selected == 6 ) {
            sctx->time.tm_mon --;
            if ( sctx->time.tm_mon < 1 ) sctx->time.tm_mon = 12;
        } else if ( sctx->selected == 7 ) {            
            sctx->time.tm_year --;
            if ( sctx->time.tm_year < 20 ) sctx->time.tm_year = 99;
        }  

        if ( sctx->time.tm_mday > getMaxDaysOfMonth( sctx->time.tm_year, sctx->time.tm_mon ) ) sctx->time.tm_mday = getMaxDaysOfMonth( sctx->time.tm_year, sctx->time.tm_mon );        
        if ( sctx->selected != 0 && sctx->selected != 4 ) RTCC_TimeSet( &sctx->time );        
    } else if ( isButtonPressed(BUTTON_UP) ) {
        sctx->oselected = sctx->selected;
        if ( sctx->selected == 4 ) { 
            sctx->selected = 0;
        } else if ( sctx->selected == 1 ) {
            sctx->time.tm_hour ++;
            if ( sctx->time.tm_hour >= 24 ) sctx->time.tm_hour = 0;
        } else if ( sctx->selected == 2 ) {
            sctx->time.tm_min ++;
            if ( sctx->time.tm_min >= 60 ) sctx->time.tm_min = 0;                        
        } else if ( sctx->selected == 3 ) {            
            sctx->time.tm_sec ++;
            if ( sctx->time.tm_sec >= 60 ) sctx->time.tm_sec = 0;
        } else if ( sctx->selected == 5 ) {
            sctx->time.tm_mday ++;         
            if ( sctx->time.tm_mday > getMaxDaysOfMonth( sctx->time.tm_year, sctx->time.tm_mon ) ) sctx->time.tm_mday = 1;
        } else if ( sctx->selected == 6 ) {            
            sctx->time.tm_mon ++;
            if ( sctx->time.tm_mon > 12 ) sctx->time.tm_mon = 1;
        } else if ( sctx->selected == 7 ) {            
            sctx->time.tm_year ++;
            if ( sctx->time.tm_year > 99 ) sctx->time.tm_year = 20;            
        }                 
             
        if ( sctx->time.tm_mday > getMaxDaysOfMonth( sctx->time.tm_year, sctx->time.tm_mon ) ) sctx->time.tm_mday = getMaxDaysOfMonth( sctx->time.tm_year, sctx->time.tm_mon );        
        if ( sctx->selected != 0 && sctx->selected != 4 ) RTCC_TimeSet( &sctx->time );
    } else if ( isButtonPressed( BUTTON_LEFT ) ) {
        sctx->oselected = sctx->selected;
        if ( sctx->selected != 0 && sctx->selected != 4 ) sctx->selected --;         
    } else if ( isButtonPressed( BUTTON_RIGHT ) ) {
        sctx->oselected = sctx->selected;
        if ( sctx->selected != 3 && sctx->selected != 7 ) sctx->selected ++;        
    } else if ( isButtonPressed( BUTTON_B ) ) {
        replaceContext(ctx, ENTRY_OVERVIEW );        
    }
}


void updatePowerState( APP_CONTEXT* ctx ) {
    if ( ctx->adc_rw_state == 0 ) {                
        if (ADC1_IsConversionComplete(VOLT_READ)) {            
            uint16_t val = ADC1_ConversionResultGet(VOLT_READ);
            ctx->power = (val >> 4) + 1;
            adcTimer = 500;
            ctx->adc_rw_state = 1;
            disableVoltPower();
            ctx->rinf_pwr = ANIMATION;
        }
    } else if ( adcTimer == 0 ) {        
        enableVoltPower();
        ctx->adc_rw_state = 0;
        ADC1_SoftwareTriggerDisable();
    }
}

void updateContext(APP_CONTEXT* ctx) {
    updateButtons(false);
    if (SENSE_CASE_GetValue()) {        
        setSleep(ctx);        
    }

    spi_fat_open(); //Open FAT SPI while handle logic 
    
    if ( ctx->ctxtype != INITIAL && ctx->ctxtype != PIN_INPUT ) {
        
        if ( device_options.umode == USB_MODE_KEYBOARD && USB_BUS_SENSE ) {
            if(USBDeviceState == DETACHED_STATE) {
                USBDeviceAttach();
            }
            USB_Interface_Tasks();
        } else {
            if(USBDeviceState != DETACHED_STATE) {
                USBDeviceDetach();
            }
        }
                 
        
        if (SDCard_CD_GetValue()) {
            //SD-Card removed       
            if (ctx->ctxtype != ERROR_SD_CD) {                
                setContext(ctx, ERROR_SD_CD);
            }
            ctx->fsmounted = false;
        } else {
            //SD-Card inserted
            if (!ctx->fsmounted) {
                mountFS( ctx );
                if ( !ctx->fsmounted ) {
                    setContext(ctx, ERROR_SD_CD);
                }
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
        case ENTRY_DETAIL:
            hctxEntryDetail(ctx);
            break;
        case VIEW_TOKEN:
            hctxViewToken(ctx);
            break;
        case EDIT_ENTRY:
            hctxEditEntry(ctx);        
            break;
        case MESSAGEBOX:            
            hctxMessagebox(ctx);        
            break;      
        case CHOOSEBOX:
            hctxChoosebox(ctx);
            break;
        case OPTIONS1:
            hctxOptions1(ctx);        
            break;                 
        case OPTIONS2:
            hctxOptions2(ctx);        
            break;              
    }
    
    spi_fat_close(); //Close FAT SPI while handle logic               
}
