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
#include "mcc_generated_files/ext_int.h"
#include "mcc_generated_files/interrupt_manager.h"

#include "fs/diskio.h"
#include "fs/ff.h"
#include "display_driver.h"
#include "sha/sha.h"

#include "usb/usb.h"
#include "usb/usb_tasks.h"


#define MODIFY_BUFFER_SIZE 256

static uint8_t pin[MAX_PIN_SIZE];
static uint8_t pinerr = 0;
static bool pinset = false;

static uint8_t masterkey[16];
static bool keyset = false;
static bool keyencr = false;


const uint8_t tetris_piecesize[] = {
    4 /* I */, 2 /* O */, 3 /* T */, 3 /* L */, 3 /* J */, 3 /* Z */, 3 /* S */
};

const uint8_t tetris_pieces[] = {
     // I
        0,0,0,0,
        1,1,1,1,
        0,0,0,0,
        0,0,0,0,
    
     // O
        1,1,0,0,
        1,1,0,0,
        0,0,0,0,
        0,0,0,0,
        
     // T
        0,1,0,0,
        1,1,1,0,
        0,0,0,0,
        0,0,0,0,
    
     // L
        0,0,1,0,
        1,1,1,0,
        0,0,0,0,
        0,0,0,0,
    
     // J
        1,0,0,0,
        1,1,1,0,
        0,0,0,0,
        0,0,0,0,
    
     // Z
        1,1,0,0,
        0,1,1,0,
        0,0,0,0,
        0,0,0,0,
    
    // S
        0,1,1,0,
        1,1,0,0,
        0,0,0,0,
        0,0,0,0   
};


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
    { NEW, TEXT_IOHEAD_NEW, TEXT_IOHEAD_NEW, TEXT_EMPTY },
    { NAME, TEXT_IOHEAD_NAME, TEXT_VIEWHEAD_NAME, TEXT_TKNAME_NAME  },
    { USER, TEXT_IOHEAD_USER, TEXT_VIEWHEAD_USER, TEXT_TKNAME_USER  },
    { KEY1, TEXT_IOHEAD_KEY, TEXT_VIEWHEAD_KEY, TEXT_TKNAME_KEY1  },
    { KEY2, TEXT_IOHEAD_KEY2, TEXT_VIEWHEAD_KEY2, TEXT_TKNAME_KEY2 },
    { URL, TEXT_IOHEAD_URL, TEXT_VIEWHEAD_URL, TEXT_TKNAME_URL  },
    { INFO, TEXT_IOHEAD_INFO, TEXT_VIEWHEAD_INFO, TEXT_TKNAME_INFO },
    { CHECK, TEXT_EMPTY, TEXT_EMPTY, TEXT_EMPTY },
};

DEVICE_OPTIONS device_options = {
    COLOR_YELLOW,
    COLOR_RED,
    100,
    USB_MODE_OFF
};

bool isPinSet() {
    return pinset;
}

bool isKeySet() {
    return keyset;
}

bool isKeyEncr() {
    return keyencr;
}

void setKeyEncr(bool encr) {
    keyencr = encr;
}


void setMasterKey(uint8_t *key) {
    keyset = true;
    keyencr = false;
    memcpy(masterkey, key, 16);
}

/*
void setMasterTestKey( ) {
    uint8_t testkey[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};        
    setMasterKey(testkey);    
}
*/

uint8_t* getMasterKey() {
    return masterkey;
}

void swipeKeys() {
    //Securely Shredd the keys by alternating write 0x00 and 0xff into there memory
    for ( int i=1;i<5;i++) {
        uint8_t v = (i%2) * 0xff;        
        memset(masterkey, v, 16);
        memcpy((void*) &CRYKEY0, masterkey, 16);
        memset(pin, v, MAX_PIN_SIZE);
    }

    keyset = false;
    keyencr = false;
    pinerr = 0;    
}

uint8_t verifyMasterKey( ) {        
    UINT rwbytes;
    uint8_t data[48];
    uint8_t cipher[16];
    uint8_t iv[16];

    FIL file;
    FILINFO fno;
    
    uint8_t result = 0;
    
    if ( f_stat( "VERIFY", &fno ) == FR_OK && f_open(&file, "VERIFY", FA_READ ) == FR_OK && fno.fsize == 48 ) {                        
        if ( f_read(&file, data,48, &rwbytes) == FR_OK && rwbytes == 48 ) {
            memset(&iv, 0x00, 16);

            //decode data
            prepareAES128BitCBC();                    
            prepare128bitDecryption( iv );

            for (int i=0;i<3;i++) {
                decrypt128bit( data+(i*16), data+(i*16) );                
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
        result = 1;
    }
    
/*
        if ( createInitialStructure ) {
            if ( f_open(&file, "VERIFY", FA_WRITE | FA_CREATE_ALWAYS ) == FR_OK ) {

                //create new verifikation file
                generateRND(data);
                generateRND(data+16);

                memset(&iv, 0x00, 16);
                for (int i=0;i<16;i++) {
                    data[i+32] = data[i] ^ data[i+16];                
                }

                //encrypt all 3 blocks
                prepareAES128BitCBC();            
                prepare128bitEncryption( iv );

                for (int i=0;i<3;i++) {
                    encrypt128bit( data+(i*16), data+(i*16) );                
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
        } else {
            result = 1;
        }
    }
 */
    
    return result;
}

uint8_t fsCreateInitialKeystoreStructure() {
    UINT rwbytes;
    uint8_t data[48];
    uint8_t iv[16];

    FIL file;
    FILINFO fno;
    
    uint8_t result = 0;    
    
    
    if ( f_stat("KS", &fno) == FR_OK && !( fno.fattrib & AM_DIR ) ) {
        //if KS is a file rather than a directory, delete it
        f_unlink("KS");
    }
    
    if ( f_stat("KS", &fno) != FR_OK ) {
        //create Keystore directory if not exits
        f_mkdir( "KS" );
        if ( f_stat("KS", &fno) == FR_OK && ( fno.fattrib & AM_DIR ) ) {
            result = 0;
        } else {
            result = 2;
        }
    }
    
    if ( result ==  0 ) {    
        if ( f_open(&file, "VERIFY", FA_WRITE | FA_CREATE_ALWAYS ) == FR_OK ) {
            //create new verifikation file
            generateRND(data);
            generateRND(data+16);

            memset(&iv, 0x00, 16);
            for (int i=0;i<16;i++) {
                data[i+32] = data[i] ^ data[i+16];                
            }

            //encrypt all 3 blocks
            prepareAES128BitCBC();            
            prepare128bitEncryption( iv );

            for (int i=0;i<3;i++) {
                encrypt128bit( data+(i*16), data+(i*16) );                
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
   
    
    return result;
}

bool hasFsKeystoreStructureAvailable() {
    FIL file;
    FILINFO fno;
          
    //Directory available
    if ( f_stat("KS", &fno) == FR_OK && ( fno.fattrib & AM_DIR ) ) {
        //Verification file available and correct length
        if ( f_stat( "VERIFY", &fno ) == FR_OK && 
             f_open(&file, "VERIFY", FA_READ ) == FR_OK && 
             fno.fsize == 48 ) {      

            return true;
        }
    }
    
    return false;
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
        case SEARCH_ENTRIES:
            return sizeof(CTX_SEARCH_ENTRIES);
        case MESSAGEBOX:
            return sizeof(CTX_MSG_BOX);
        case CHOOSEBOX:
            return sizeof(CTX_CHOOSE_BOX);           
        case ENTRY_DETAIL:
            return sizeof(CTX_ENTRY_DETAIL);
        case USB_PUSH:
            return sizeof(CTX_USB_PUSH);            
        case VIEW_TOKEN:
            return sizeof(CTX_VIEW_TOKEN);  
        case DEVICEOPTIONS:
            return sizeof(CTX_DEVICEOPTIONS);  
        case PINOPTIONS:
            return sizeof(CTX_PINOPTIONS);                                      
        case GAME_TETRIS:
            return sizeof(CTX_TETRIS);                                                  
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

void tet_getNextStone( CTX_TETRIS *ctx ) {
    //generate a random stone with the TRUE RANDOM generator of the controller, so no one can complain :-)
    uint8_t rnd[16];
    uint8_t i = 0;
        
    while ( true ) {
        if ( i == 0 ) generateRND(rnd);
        if ( rnd[i] < 252 ) {
            ctx->next = rnd[i] % 7;
            break;
        } else {
            i++;
            if ( i >= 16 ) i=0;
        }
    }
}

void tet_getRotatedShape( uint8_t *shape, TETRIS_STONES piece, uint8_t rot ) {
    //get the shape of a specifies stone with a specified rotation
    
    memcpy(shape, tetris_pieces + (piece*16), 16 );    

    uint8_t size = tetris_piecesize[piece];
	uint8_t ssize = size - 1;
	uint8_t s1, s2, s3, s4, i, j, k, len, offs;
    
    for (i=0;i<rot;i++) {
        len = size;
        offs = 0;                
        for (j=0;j<size/2;j++) {
            for (k=offs;k<len-1;k++) {
                s1 = shape[j+k*4];
				s2 = shape[k+(ssize-j)*4];
				s3 = shape[ssize-j+(ssize-k)*4];
				s4 = shape[ssize-k+(j*4)];
				
				shape[j+k*4] = s4;
				shape[k+(ssize-j)*4] = s1;
				shape[ssize-j+(ssize-k)*4] = s2;
				shape[ssize-k+(j*4)] = s3;
            }
			len -= 1;
			offs += 1;			
        }
    }
 
}


uint8_t tet_checkCollision( CTX_TETRIS *ctx, int x, int y, int rot ) {    
    uint8_t shape[16];
    uint8_t size = tetris_piecesize[ctx->curr];
    uint8_t ret = 0;
    
    //Check current stone on a specified position on the field
    //to get "wallkick" abilities we return different values for left and right wall collision
        
    tet_getRotatedShape(shape, ctx->curr, rot );
    for (int cx=0;cx<size;cx++) {
        for (int cy=0;cy<size;cy++) {
            int fx = x + cx;
            int fy = y + cy;
            if ( shape[cx + cy * 4] > 0 ) {
                if ( fx < 0 ) return 2;
                if ( fx >= 10 ) return 3;
                if ( fy >= 20 ) return 1;
                if ( ctx->field[fx + fy * 10] != TETS_EMPTY ) ret = 1;
            }            
        }
    }
                   
    return ret;    
}

bool tet_placeNextStone( CTX_TETRIS *ctx ) {                 
    //place next stone on the field
    //if it starts with an collision its gameover
    
    ctx->currx = ( 10 - tetris_piecesize[ctx->next] ) / 2;
    ctx->curry = 0;
    ctx->currrot = 0;
       
    ctx->curr = ctx->next;
    tet_getNextStone( ctx );
    if ( tet_checkCollision( ctx, ctx->currx, ctx->curry, ctx->currrot ) != 0 ) {
        return false;
    }
    
    return true;          
}

void tet_positionStone( CTX_TETRIS *ctx ) {    
    uint8_t shape[16];
    uint8_t size = tetris_piecesize[ctx->curr];
    
    //position the current shape in the field    
    tet_getRotatedShape(shape, ctx->curr, ctx->currrot );
    for (int cx=0;cx<size;cx++) {
        for (int cy=0;cy<size;cy++) {
            int fx = ctx->currx + cx;
            int fy = ctx->curry + cy;
            if ( shape[cx + cy * 4] > 0 ) {
                ctx->field[fx + fy * 10] = ctx->curr;
            }
        }
    }
    
    //check for filled lines
    uint8_t linecount=0;
    for (int fy=0;fy<20;fy++) {
        bool filled = true;
        for (int fx=0;fx<10;fx++) {
            if ( ctx->field[fx + fy * 10] == TETS_EMPTY ) {
                filled = false;
                break;
            }
        }
        if ( filled ) {
            linecount++;
            if ( fy > 0 ) {                
                memmove( &ctx->field[10], ctx->field, fy * 10 *sizeof(TETRIS_STONES) );
            }
            for ( int i=0;i<10;i++) ctx->field[i] = TETS_EMPTY;
        }
    }
    
    if ( linecount > 0 ) {
        uint32_t smult = 10 + ctx->lvl * 2;    
        uint32_t s = linecount;
        ctx->lines += s;
                                
        ctx->currlvllines += linecount;
        if ( ctx->currlvllines > 10 ) {
            ctx->currlvllines-= 10;
            ctx->lvl ++;
        }
        
        switch ( linecount) {
            case 1:
                s=100;
                ctx->b2btetris = false;
                break;
            case 2:
                s=300;
                ctx->b2btetris = false;
                break;
            case 3:
                s=500;
                ctx->b2btetris = false;
                break;
            case 4:                   
                if ( ctx->b2btetris ) {
                    s = 1200;
                } else {
                    s=800;
                }
                ctx->b2btetris = true;
                break;
        }
        ctx->score += ( s * smult ) / 10;
    }    
}


void tet_runDropTimer( CTX_TETRIS *ctx ) {
    const int quicktimer = 25;
    uint32_t lvl = ctx->lvl;        
    
    gameTimer = 300 * 5 / ( 5 + lvl ) ; 
                
    if ( isButtonDown(BUTTON_RIGHT) ) {        
        gameTimer = gameTimer > quicktimer ? quicktimer : gameTimer;        
    }
}

void tet_initialize( CTX_TETRIS *ctx ) { 
    for ( int i=0;i<200;i++) ctx->field[i] = TETS_EMPTY;
    ctx->lines = 0;
    ctx->lvl = 0;
    ctx->score = 0;
    ctx->currrot = 0;
    ctx->gameover = false;
    ctx->startpause = true;
    ctx->b2btetris = false;
    ctx->currlvllines = 0;
    
    tet_getNextStone( ctx );
    tet_placeNextStone( ctx );
}


void hctxTetris( APP_CONTEXT *ctx ) {
    CTX_TETRIS *sctx;
    sctx = (CTX_TETRIS*) (ctx->ctxbuffer + ctx->ctxptr);     
        
    if ( isButtonPressed(BUTTON_B) ) {
        setContext(ctx, ENTRY_OVERVIEW);            
        return;
    }


    if ( sctx->startpause ) {
        if ( isButtonPressed(BUTTON_A) ) {
            if ( sctx->gameover ) {                
                tet_initialize( sctx );
            } else {                            
                tet_runDropTimer( sctx );
                sctx->startpause = false;
            }
            ctx->rinf = ANIMATION;
        }
    } else {
        if ( isButtonPressed(BUTTON_DOWN) ) {
            if ( !tet_checkCollision( sctx, sctx->currx - 1, sctx->curry, sctx->currrot ) ) {
                sctx->currx --;
                ctx->rinf = ANIMATION2;
            }
        }
        if ( isButtonPressed(BUTTON_UP) ) {
            if ( !tet_checkCollision( sctx, sctx->currx + 1, sctx->curry, sctx->currrot ) ) {
                sctx->currx ++;
                ctx->rinf = ANIMATION2;
            }
        }
        if ( isButtonPressed(BUTTON_RIGHT) ) {
            tet_runDropTimer( sctx );      
        }
        if ( isButtonPressed(BUTTON_LEFT) ) {
            int dx = 0;
            while ( true ) {
                uint8_t nextrot = sctx->currrot < 3 ? sctx->currrot + 1 : 0;                                
                uint8_t cinf = tet_checkCollision( sctx, sctx->currx + dx, sctx->curry, nextrot );
                
                if ( cinf == 2 ) { 
                    //check wallkick left
                    dx ++;
                    continue;
                }
                if ( cinf == 3 ) { 
                    //check wallkick right
                    dx --;
                    continue;
                }
                if ( cinf == 0 ) {
                    //normal roation
                    sctx->currrot = nextrot;
                    sctx->currx += dx;
                    ctx->rinf = ANIMATION2;
                }   
                break;
            }
        }
        
        if ( !gameTimer ) {
            ctx->rinf = ANIMATION2;
                        
            if ( tet_checkCollision( sctx, sctx->currx, sctx->curry + 1, sctx->currrot ) ) {
                //place Stone  
                ctx->rinf = ANIMATION;
                tet_positionStone( sctx );
                if ( !tet_placeNextStone( sctx ) ) {
                    sctx->gameover = true;
                    sctx->startpause = true;
                } else {
                    tet_runDropTimer( sctx );
                }
            } else {
                sctx->curry ++;
                tet_runDropTimer( sctx );
            }                 
        }
    }
    
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
    if ( !del ) {
        if ( ioctx->inp == TOKEN || 
             ioctx->inp == MASTERKEY_GEN || 
             ioctx->inp == MASTERKEY_VERIFY ||
             ioctx->inp == MASTERKEY_CHECK ) {
            if ( ioctx->tlen >= MAX_TEXT_LEN ) return 1;                
        } else if ( ioctx->inp == SEARCHFIELD ) {
            if ( ioctx->tlen >= MAX_SEARCH_LEN ) return 1;
        }
    }
    
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
            if ( ioctx->mboxresult == MSGB_EDIT_ABORT_YES ) {
                return IOR_ABORT;
            } else if ( ioctx->mboxresult == MSGB_EDIT_ACCEPT_YES ) {
                Keylayout *k = getCurrentKeyboardKey(ioctx);
                if ( k->fkt == KEYCOMMAND_OK ) {
                    memmove( ioctx->text, ioctx->text+1, ioctx->tlen ); //remove startlinefeed and add zero termination char to the end to save it properly on disk
                    ioctx->text[ioctx->tlen-1] = 0x00;
                    return IOR_OK;
                }
            }
            ioctx->mboxresult = MSGB_NO_RESULT;
        } else if ( isButtonPressed(BUTTON_A) ) {                                    
            Keylayout *k = getCurrentKeyboardKey(ioctx);

            if ( k->fkt == KEYCOMMAND_CHAR || k->fkt == KEYCOMMAND_SPACE || k->fkt == KEYCOMMAND_LF ) {
                uint8_t cid=CHAR_SPACE;
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
            } else if ( k->fkt == KEYCOMMAND_GEN && ioctx->inp == TOKEN ) {                
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
            } else if ( k->fkt == KEYCOMMAND_ABORT && ioctx->inp == TOKEN ) {
                if ( ioctx->datachanged ) {
                    pushContext(ctx, MESSAGEBOX);
                    CTX_MSG_BOX *mctx;
                    mctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);
                    
                    mctx->mchoices[0] = MSGB_EDIT_ABORT_YES;
                    mctx->mchoiceicon[0] = SYSICON_OK;
                    mctx->mchoices[1] = MSGB_EDIT_ABORT_NO;
                    mctx->mchoiceicon[1] = SYSICON_ABORT;
                    mctx->choicecount = 2;
                    mctx->sel = 0;
                                                         
                    memcpy(mctx->msgtxt, (char*)(textdata + texte[TEXT_MSG_ABORT]), strlen( (char*)(textdata + texte[TEXT_MSG_ABORT]) ) + 1 );
                                        
                    return IOR_CTX_SWITCH;
                } else {
                    return IOR_ABORT;
                }
            } else if ( k->fkt == KEYCOMMAND_OK ) {
                if ( ioctx->inp == TOKEN ) {
                    if ( ioctx->datachanged && ioctx->type != NEW ) {
                        pushContext(ctx, MESSAGEBOX);
                        CTX_MSG_BOX *mctx;
                        mctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);

                        mctx->mchoices[0] = MSGB_EDIT_ACCEPT_YES;
                        mctx->mchoiceicon[0] = SYSICON_OK;
                        mctx->mchoices[1] = MSGB_EDIT_ACCEPT_NO;
                        mctx->mchoiceicon[1] = SYSICON_ABORT;
                        mctx->choicecount = 2;
                        mctx->sel = 0;

                        memcpy(mctx->msgtxt, (char*)(textdata + texte[TEXT_MSG_OVERWRITE]), strlen( (char*)(textdata + texte[TEXT_MSG_OVERWRITE]) ) + 1 );

                        return IOR_CTX_SWITCH;
                    } else {                    
                        memmove( ioctx->text, ioctx->text+1, ioctx->tlen ); //remove startlinefeed and add zero termination char to the end to save it properly on disk
                        ioctx->text[ioctx->tlen-1] = 0x00;
                        return IOR_OK;
                    }
                } else if ( ioctx->inp == MASTERKEY_GEN || 
                            ioctx->inp == MASTERKEY_VERIFY || 
                            ioctx->inp == MASTERKEY_CHECK || 
                            ioctx->inp == SEARCHFIELD ) {
                    memmove( ioctx->text, ioctx->text+1, ioctx->tlen ); //remove startlinefeed and add zero termination char to the end to save it properly on disk
                    ioctx->text[ioctx->tlen-1] = 0x00;
                    return IOR_OK;
                }
            }

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

static const uint8_t pinfield[] = {
    PFLD_TREE,        PFLD_TREE,        PFLD_TREE,        PFLD_GROUND,      PFLD_SWORD,       PFLD_STONE,       PFLD_STONE,       PFLD_STONE,       
    PFLD_TREE,        PFLD_OWL,         PFLD_TREE,        PFLD_GROUND,      PFLD_GROUND,      PFLD_STONE,       PFLD_STONE,       PFLD_DRAGON,       
    PFLD_TREE,        PFLD_TREE,        PFLD_TREE,        PFLD_GROUND,      PFLD_GROUND,      PFLD_STONE,       PFLD_STONE,       PFLD_STONE,       
    PFLD_GROUND,      PFLD_TREE,        PFLD_GROUND,      PFLD_TREE,        PFLD_GROUND,      PFLD_GROUND,      PFLD_STONE,       PFLD_LITTLEDRAGON,       
    PFLD_GROUND,      PFLD_TREE,        PFLD_TREE,        PFLD_GROUND,      PFLD_GROUND,      PFLD_GROUND,      PFLD_STONE,       PFLD_STONE,       
    PFLD_SHIELD,      PFLD_GROUND,      PFLD_TREE,        PFLD_GROUND,      PFLD_GROUND,      PFLD_GROUND,      PFLD_GROUND,      PFLD_STONE,       
    PFLD_GROUND,      PFLD_STONE,       PFLD_GROUND,      PFLD_GROUND,      PFLD_GROUND,      PFLD_GROUND,      PFLD_STONE,       PFLD_STONE,       
    PFLD_GROUND,      PFLD_GROUND,      PFLD_GROUND,      PFLD_GROUND,      PFLD_GROUND,      PFLD_STONE,       PFLD_STONE,       PFLD_GRAIL,    
};

void initPinInput( APP_CONTEXT* ctx ) {    
    CTX_PIN_INPUT *sctx;
    sctx = (CTX_PIN_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);
    sctx->px = 4;
    sctx->py = 4;
    sctx->extrasteps = 0;
    sctx->pinerr = pinerr;
    sctx->errorchecked = false;
    memcpy(sctx->field, &pinfield , 64 );        
}

void initKeyInput( APP_CONTEXT* ctx ) {
    CTX_KEY_INPUT *sctx;
    sctx = (CTX_KEY_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);            
}

PINFIELD getPinfieldObj(APP_CONTEXT* ctx, uint8_t x, uint8_t y) {
    CTX_PIN_INPUT *sctx;
    sctx = (CTX_PIN_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);
    
    if ( x >= 0 && x < 8 && y >= 0 && y < 8 ) {
        return sctx->field[x + y * 8];
    }
    return PFLD_NOTHING;
}

void setPinfieldObj(APP_CONTEXT* ctx, int8_t x, int8_t y, PINFIELD obj) {
    CTX_PIN_INPUT *sctx;
    sctx = (CTX_PIN_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);
    
    if ( x >= 0 && x < 8 && y >= 0 && y < 8 ) {
        sctx->field[x + y * 8] = obj;
    }    
}



void hctxPinInput(APP_CONTEXT* ctx) {
    CTX_PIN_INPUT *sctx;
    sctx = (CTX_PIN_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);

    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
        if (isButtonPressed(i)) {
            ctx->rinf = ANIMATION;         

            if ( !sctx->error ) {
                if ( sctx->ppos < device_options.pin_len ) {
                    if (sctx->verify) {
                        sctx->verifypin[ sctx->ppos ] = i;
                    } else {
                        sctx->pin[ sctx->ppos ] = i;
                    }
                    sctx->ppos++;
                }
                //sctx->haderror = sctx->error;
                                
                int8_t plnx = sctx->px;
                int8_t plny = sctx->py;
                
                switch (i) {
                    case BUTTON_LEFT:
                        if ( plnx > 0 ) plnx --;
                        break;
                    case BUTTON_RIGHT:
                        if ( plnx < 7 ) plnx ++;
                        break;
                    case BUTTON_UP:
                        if ( plny > 0 ) plny --;
                        break;
                    case BUTTON_DOWN:
                        if ( plny < 7 ) plny ++;                    
                        break;
                    case BUTTON_A:
                    {
                        int8_t plhx = sctx->px;
                        int8_t plhy = sctx->py;                        
                        
                        for ( int8_t x=plhx-1;x<=plhx+1;x++ ) {
                            for ( int8_t y=plhy-1;y<=plhy+1;y++ ) {
                                PINFIELD obj = getPinfieldObj(ctx, x, y );
                                if ( obj == PFLD_TREE || obj == PFLD_CRACKSTONE ) {
                                    setPinfieldObj(ctx, x, y, PFLD_GROUND );
                                } else if ( obj == PFLD_STONE ) {
                                    setPinfieldObj(ctx, x, y, PFLD_CRACKSTONE );
                                }
                            }
                        }                                                                                                  
                    }
                        break;
                    case BUTTON_B:
                        break;
                }

                if ( plnx != sctx->px || plny != sctx->py ) {
                    PINFIELD obj = getPinfieldObj(ctx, plnx, plny );
                    
                    if ( obj == PFLD_OWL ) sctx->extrasteps += 3;
                    if ( obj == PFLD_SHIELD ) sctx->extrasteps += 4;
                    if ( obj == PFLD_LITTLEDRAGON ) sctx->extrasteps += 2;
                    if ( obj == PFLD_DRAGON ) sctx->extrasteps += 1;
                    if ( obj == PFLD_SWORD ) sctx->extrasteps += 1;
                    if ( obj == PFLD_GRAIL ) sctx->extrasteps += 3;
                    
                    if ( obj == PFLD_GROUND || 
                         obj == PFLD_OWL || 
                         obj == PFLD_LITTLEDRAGON || 
                         obj == PFLD_SHIELD || 
                         obj == PFLD_DRAGON ||
                         obj == PFLD_SWORD ||
                         obj == PFLD_GRAIL ) {
                        
                        setPinfieldObj(ctx, plnx, plny, PFLD_GROUND );
                        sctx->px = plnx;
                        sctx->py = plny;
                    }
                }
    

                if (sctx->ppos == device_options.pin_len ) {
                    if (sctx->generatenew) {
                        if (sctx->verify) {
                            if (memcmp(sctx->pin, sctx->verifypin, device_options.pin_len) == 0) {
                                //Pin match verification pin, set new pin and go to overview page
                                memcpy(pin, sctx->pin, device_options.pin_len);
                                pinerr = 0;
                                pinset = true;
                                removeContext(ctx);
                                //setContext(ctx, ENTRY_OVERVIEW);
                            } else {
                                //Pin differs from verification pin, retry                                
                                sctx->error = true;
                            }
                        } else {
                            initPinInput( ctx );                            
                            sctx->verify = true;
                            sctx->ppos = 0;                                
                        }
                    } else {
                        if (memcmp(sctx->pin, pin, device_options.pin_len) == 0) {
                            pinerr = 0;
                            setContext(ctx, VERIFY_KEY_AFTER_PIN);
                        } else {
                        
                            if ( !sctx->errorchecked ) {
                                sctx->errorchecked = true;                                 
                                if ( sctx->extrasteps == 0 ) {
                                    sctx->error = true;
                                }
                                
                                pinerr++;
                                if (pinerr == device_options.pin_tries) {
                                    pinerr = 0;
                                    swipeKeys();
                                    setContext(ctx, KEY_INPUT);
                                    initKeyInput( ctx );
                                }
                            } else {                                
                                sctx->extrasteps--;
                                if ( sctx->extrasteps == 0 ) {
                                    sctx->error = true;
                                }  
                            }
                        }
                    }
                }
                break;
            } else {
                sctx->ppos = 0;
                sctx->error = false;                
                initPinInput( ctx );
                sctx->verify = false;
                sctx->ppos = 0;
                memset(sctx->pin, 0x00, device_options.pin_len);
                memset(sctx->verifypin, 0x00, device_options.pin_len);                                                                            
                ctx->rinf = REFRESH;
            }
        }
    }

}

void hctxVerifyKeyAfterPin(APP_CONTEXT* ctx) {
                
    if ( isKeySet() && isKeyEncr() ) { 
        //If the key is encrypted in nmemory, decrypt it for first
        //The random key for decryption of the masterkey is still in the crykey register
        //of the cryptografic engine, no further initialization before decryption needed
                
        uint8_t iv[16];        
        uint8_t* mkeyp;
        
        memset(iv, 0x00, sizeof(iv) );
        
        CRYCONLbits.CRYON = 1;
        
        mkeyp = getMasterKey();
        decrypt128bit( mkeyp, mkeyp );
        
        CRYCONLbits.CRYON = 0;
        
        setKeyEncr(false);
    }    
    
    uint8_t keyres = verifyMasterKey( );
    if ( keyres == 0 ) {
        setContext(ctx, ENTRY_OVERVIEW);
    } else if ( keyres == 1 ) {
        swipeKeys();
        setContext(ctx, KEY_INPUT);
        initKeyInput( ctx );
    } else if ( keyres == 2 ) {
        setContext(ctx, ERROR_SD_FAILURE);
    }    
    
}

void calcKeyFromText( uint8_t* text, uint16_t textlen, uint8_t* key ) {
    SHA256_CTX ctx;
    uint8_t hash[32];
    int i;       
    
    sha256_init(&ctx);
    for (i=0;i<textlen;i++) {
        hash[0] = gfxchars[ text[i] ].uccp >> 8;
        hash[1] = gfxchars[ text[i] ].uccp & 0xff;
        
        sha256_update(&ctx, hash, 2 );
    }                            
    sha256_final(&ctx, hash );
    memcpy(key, hash, 16 );
}


void hctxKeyInput(APP_CONTEXT* ctx) {
    CTX_KEY_INPUT *sctx;
    sctx = (CTX_KEY_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);

    if ( !sctx->fschecked ) {
        sctx->fschecked = true;
        
        if ( !hasFsKeystoreStructureAvailable() ) {
            sctx->generatemode = true;            
            sctx->io.inp = MASTERKEY_GEN;
        } else {
            sctx->io.inp = MASTERKEY_CHECK; 
        }
                
        sctx->io.type = NEW;
        sctx->io.rinf = REFRESH;
        sctx->io.kbmap = DEFAULT_KEYMAP;
        sctx->io.text[0] = CHAR_LINEFEED;
        sctx->io.tlen = 1;
        sctx->io.tewp = 1;        
        ctx->rinf = IO_UPDATE;     
                
    } else {            
        sctx->haderror = sctx->error;
        HCTX_IO_RESULT res = hctxIO( &sctx->io, ctx );

        if ( res == IOR_UNCHANGED ) {
            ctx->rinf = UNCHANGED;
        } else if ( res == IOR_UPDATED ) {
            ctx->rinf = IO_UPDATE;
        } else if ( res == IOR_OK ) {
            uint8_t newkey[16];
                       
            if ( sctx->io.tlen > 1 ) {   
                    
                if ( sctx->generatemode ) {
                    if ( sctx->io.inp == MASTERKEY_GEN ) {                        
                        calcKeyFromText( sctx->io.text, sctx->io.tlen, sctx->generatedkey );
                        sctx->io.inp = MASTERKEY_VERIFY;
                        sctx->io.rinf = REFRESH;
                        sctx->io.kbmap = DEFAULT_KEYMAP;
                        sctx->io.text[0] = CHAR_LINEFEED;
                        sctx->io.tlen = 1;
                        sctx->io.tewp = 1;
                        ctx->rinf = IO_UPDATE;                          
                    } else {
                        calcKeyFromText( sctx->io.text, sctx->io.tlen, newkey );
                        if ( memcmp( sctx->generatedkey, newkey, 16 ) == 0 ) {
                            setMasterKey(newkey);
                            
                            fsCreateInitialKeystoreStructure();
                            
                            setContext(ctx, ENTRY_OVERVIEW);
                            pushContext(ctx, PIN_INPUT);
                            CTX_PIN_INPUT *pctx;
                            pctx = (CTX_PIN_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);
                            pctx->generatenew = 1;
                            initPinInput( ctx );                            
                        } else {

                            sctx->io.inp = MASTERKEY_GEN;
                            sctx->io.rinf = REFRESH;
                            sctx->io.kbmap = DEFAULT_KEYMAP;
                            sctx->io.text[0] = CHAR_LINEFEED;
                            sctx->io.tlen = 1;
                            sctx->io.tewp = 1;
                                                        
                            pushContext(ctx, MESSAGEBOX);
                            CTX_MSG_BOX *mctx;
                            mctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);

                            mctx->mchoices[0] = MSGB_MKEY_ERROR_OK;
                            mctx->mchoiceicon[0] = SYSICON_OK;
                            mctx->choicecount = 1;
                            mctx->sel = 0;

                            memcpy(mctx->msgtxt, (char*)(textdata + texte[TEXT_ENTER_NEW_KEY_ERROR]), strlen( (char*)(textdata + texte[TEXT_ENTER_NEW_KEY_ERROR]) ) + 1 );                             
                        }
                    }

                } else {                                                            
                    calcKeyFromText( sctx->io.text, sctx->io.tlen, newkey );
                    setMasterKey(newkey);

                    uint8_t keyres = verifyMasterKey( );
                    if ( keyres == 0 ) {
                        setContext(ctx, ENTRY_OVERVIEW);
                        pushContext(ctx, PIN_INPUT);
                        CTX_PIN_INPUT *pctx;
                        pctx = (CTX_PIN_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);
                        pctx->generatenew = 1;
                        initPinInput( ctx );
                    } else if ( keyres == 1 ) {
                        //add startlinefeed again to prepare the editor
                        memmove( sctx->io.text+1, sctx->io.text, sctx->io.tlen ); 
                        sctx->io.text[0] = CHAR_LINEFEED;

                        pushContext(ctx, MESSAGEBOX);
                        CTX_MSG_BOX *mctx;
                        mctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);

                        mctx->mchoices[0] = MSGB_MKEY_ERROR_OK;
                        mctx->mchoiceicon[0] = SYSICON_OK;
                        mctx->choicecount = 1;
                        mctx->sel = 0;

                        memcpy(mctx->msgtxt, (char*)(textdata + texte[TEXT_ENTER_KEY_ERROR]), strlen( (char*)(textdata + texte[TEXT_ENTER_KEY_ERROR]) ) + 1 );

                        ctx->rinf = REFRESH;
                    } else if ( keyres == 2 ) {
                        setContext(ctx, ERROR_SD_FAILURE);
                    }                
                }
            } else {
                memmove( sctx->io.text+1, sctx->io.text, sctx->io.tlen );
            }
            
        }        
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
    int i, j, k;    
    uint8_t tokenpos;
    FSIZE_t readoffs;
    uint32_t filelen;
    
    res = f_stat( path, &fno);
    filelen = fno.fsize;
    if ( res == FR_OK && filelen >= 48  && filelen % 16 == 0 ) {
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
            readoffs = tokenpos;
            
            if ( tokenpos == 0xff || filelen < ( readoffs * TOKEN_BLOCK_SIZE + 48 ) ) {
                //Token not found, or file too small/corrupted
                //return an empty token anyway
                text[0] = 0x00;
                *tlen = 1;
                f_close(&file);
                endEncryption();
                return FR_OK;
            }
            
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
                    for (k=0;k<16;k+=2) {                        
                        uint16_t ct = buffer[j+k];
                        uint16_t ct2 = buffer[j+k+1];
                                
                        ct = ct << 8;
                        ct |= ct2;
                        
                        text[*tlen] = unicodeLookup( ct );                               
                        *tlen += 1;
                                              
                        if ( text[*tlen-1] == 0 || *tlen >= maxlen ) {
                            text[*tlen-1] = 0x00;
                            f_close(&file);
                            endEncryption();
                            return FR_OK;                             
                        }
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
    int i, j, k, l;    
    uint8_t tokenpos;
    uint8_t freetokenpos;
    uint8_t lasttokenpos;
    uint16_t tokencnt=0;
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
            } else {
                if ( len == 0 ) {
                    buffer[ tokenpos + 32] = NEW;
                }                
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
                    tokencnt = ( TOKEN_BLOCK_SIZE / TOKEN_READ_MOD_BUFFERSIZE ) * tokenpos;
                } else if ( mode == 1 ) {
                    tokencnt = TOKEN_BLOCK_SIZE / TOKEN_READ_MOD_BUFFERSIZE;
                    if ( lasttokenpos == 0xff || lasttokenpos < tokenpos ) {
                        //expand file and get back to current position
                        readoffs += TOKEN_BLOCK_SIZE;
                        f_lseek( &file, readoffs );
                        readoffs -= TOKEN_BLOCK_SIZE;
                        f_lseek( &file, readoffs );
                        lasttokenpos += 1;
                    }
                } else if ( mode == 2 ) {
                    tokencnt = ( lasttokenpos - tokenpos ) * ( TOKEN_BLOCK_SIZE / TOKEN_READ_MOD_BUFFERSIZE );
                }
                
                for ( i=0; i<tokencnt; i++ ) {
                    res = f_read( &file, buffer, TOKEN_READ_MOD_BUFFERSIZE, &affected );
                    if ( res != FR_OK || affected < TOKEN_READ_MOD_BUFFERSIZE ) return errAbortFileEntryHandling( &file, path );

                    for (k=0;k<TOKEN_READ_MOD_BUFFERSIZE;k+=16) decrypt128bit( buffer+k, buffer+k );
                    switch128BitDecryptEncrypt();                                

                    if ( mode == 1 ) {
                        //modify tokendata
                        for (k=0;k<TOKEN_READ_MOD_BUFFERSIZE;k+=2) {
                            l = i * TOKEN_READ_MOD_BUFFERSIZE + k;
                            j = l / 2;
                            
                            if ( j < len ) {
                                buffer[k]   = (uint8_t )( gfxchars[ text[j] ].uccp >> 8 );
                                buffer[k+1] = (uint8_t )( gfxchars[ text[j] ].uccp & 0xFF );
                            } else {
                                buffer[k]   = nonse[  l     % 16 ];
                                buffer[k+1] = nonse[( l+1 ) % 16 ];
                            }
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


FRESULT createNewEntry( TCHAR *path ) {  //, uint8_t *text, uint16_t len ) {    
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
    
    FRESULT res;  
    
    uint16_t tlen;
    CTX_ENTRY_DETAIL *sctx;
    sctx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);  
         
    sctx->selected = NAME;  
    sctx->tokencount = 6;
    
    for (int i=0;i<sctx->tokencount; i++) {
        sctx->tokeninfo[i].type = token_configs[i+1].type;
        sctx->tokeninfo[i].isset = false;
        
        res = readEntryToken( sctx->path, sctx->tokeninfo[i].type, sctx->tokeninfo[i].text, 48, &tlen );
        if ( res != FR_OK ) return res;   
        
        if (tlen > 1) sctx->tokeninfo[i].isset = true;
        
        if ( sctx->tokeninfo[i].type == KEY1 || sctx->tokeninfo[i].type == KEY2 ) {
            memset( sctx->tokeninfo[i].text, PASSWORD_REPLACE_CHAR, tlen-1 );
        }                                 
    }
 
    return FR_OK;
}


FRESULT hctxEntryOverviewReloadList( APP_CONTEXT* ctx ) {
    CTX_ENTRY_OVERVIEW *sctx;
    sctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);

    FRESULT res;
    DIR dir;
    static FILINFO fno;
    TCHAR path[16];
    uint16_t len;
    
    
    sctx->bufferlen = 0;
    sctx->bufferstart = 0;
    if ( sctx->entrycount == 0 ) return FR_OK;
    
    res = f_opendir(&dir, "KS");                       
    if (res == FR_OK) {
        int32_t start = sctx->cursor;
        start = start - MAX_OVERVIEW_ENTRY_COUNT / 2;
        if ( start < 0 ) start = 0;         
        uint16_t end = start + ( MAX_OVERVIEW_ENTRY_COUNT - 1 );
        if ( end >= sctx->entrycount ) {
            end = sctx->entrycount - 1;
        }
        if ( end < start ) return FR_OK;
        
        //sctx->entrycenter = sctx->cursor;
        
        uint16_t curr = 0;
        while ( true ) {
            res = f_readdir(&dir, &fno);                                                                      
            if ( curr == start ) sctx->bufferstart = curr;
            if ( curr > end ) {
                sctx->bufferlen = curr - sctx->bufferstart;
                break;
            }
            if (res != FR_OK || fno.fname[0] == 0) {
                sctx->bufferstart = 0;
                return FR_DISK_ERR;
            }
                        
            if ( curr >= start ) {
                memcpy( sctx->entries[ curr - sctx->bufferstart ].path , &fno.fname, strlen( (char*)&fno.fname ) + 1 );
                //sctx->entries[ curr - sctx->bufferstart ].loaded = false;     
            }            
            curr++;
        }
        f_closedir(&dir);
    
        for (curr = 0; curr < sctx->bufferlen; curr ++) {            
            memcpy( path, &"KS/", 3 );
            memcpy( path+3, sctx->entries[curr].path, strlen( sctx->entries[curr].path ) + 1 );
            
            if ( readEntryToken( path, NAME, sctx->entries[curr].name, 47, &len ) != FR_OK ) {
                sctx->entries[curr].name[0] = 0;
            } 
        }
    }
               
    return FR_OK;
}

/*
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
*/
  
  
bool compareEntryName(uint8_t *ename, uint8_t elen, uint8_t *searchstr, uint8_t *searchstr2, uint8_t slen, bool cs ) {
    bool res;

    if ( elen < slen ) return false;
    uint8_t clen = elen - slen;
        
    //case sensitive search, using searchstr and searchstr2  
    for (uint8_t i=0;i<clen;i++) {
        res = true;

        for (uint8_t j=0;j<slen;j++) {
            if ( ename[i+j] != searchstr[j] ) {
                if ( searchstr2[j] == 0 || ename[i+j] != searchstr2[j] ) {
                    res = false;
                    break;
                }
            }
        }
        if ( res ) return true;
    }
 
    return false;
}

uint16_t searchNextEntryFrom( CTX_ENTRY_OVERVIEW *sctx, uint16_t from ) {
    FRESULT res;
    DIR dir;
    static FILINFO fno;    
 
    uint16_t len;
    TCHAR path[16];
    
    uint8_t tokendata[48];
        
    uint16_t pos;
    uint8_t searchstr2[MAX_SEARCH_LEN];
    uint8_t slen = strlen( (char *) sctx->searchstr );
    uint8_t i, j;
    bool topsearch = true;

    for (i= 0; i<slen; i++) {
       searchstr2[i] = casetranslatelist[ sctx->searchstr[i] ];
    }        
    searchstr2[slen] = sctx->searchstr[slen]; //zero termination
 
        
    for (j=0;j<2;j++) {
        topsearch = j == 0;
        pos=0;
        
        res = f_opendir(&dir, "KS");                       
        if (res == FR_OK) {
            while ( true ) {
                res = f_readdir(&dir, &fno);                   
                if (res != FR_OK || fno.fname[0] == 0) break;  

                if ( ( pos > from && topsearch ) || ( pos <= from && !topsearch ) ) {
                    memcpy( path, &"KS/", 3 );                    
                    memcpy( path+3, fno.fname, strlen( fno.fname ) + 1 );

                    res = readEntryToken( path, NAME, tokendata, 47, &len );
                    if ( res != FR_OK ) {
                        return -2;
                    }
                    if ( compareEntryName( tokendata, (uint8_t)len, sctx->searchstr, searchstr2, slen, true ) ) {
                        return pos;
                    }
                }

                pos++;       
            }
            f_closedir(&dir);
        }        
    }
    
    return -1;
}

uint16_t countTotalEntriesOnDisk() {
    FRESULT res;
    DIR dir;
    static FILINFO fno;
    
    uint16_t count = 0;
    
    res = f_opendir(&dir, "KS");                       
    if (res == FR_OK) {
        while ( true ) {
            res = f_readdir(&dir, &fno);                   
            if (res != FR_OK || fno.fname[0] == 0) break;  
            count++;
        }
        f_closedir(&dir);
    }    
    return count;
}

void hctxEntryOverview(APP_CONTEXT* ctx) {
    CTX_ENTRY_OVERVIEW *sctx;
    sctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);
        
    if ( !sctx->initialized ) {
        sctx->initialized = true;
        
        sctx->entrycount = countTotalEntriesOnDisk();               
        if ( sctx->cursor >= sctx->entrycount ) sctx->cursor = sctx->entrycount > 0 ? sctx->entrycount - 1 : 0;
        
        if ( sctx->needinitsearch ) {
            sctx->needinitsearch = false;
            if ( strlen( (char *)sctx->searchstr ) > 0 ) { 
                uint16_t next = searchNextEntryFrom( sctx, -1 );
                if ( next == -2 ) {
                    setContext(ctx, ERROR_SD_CD);
                    return;
                } else if ( next >= 0 && next < sctx->entrycount ) {
                    sctx->cursor = next;
                    sctx->searchsuccessful = 1;
                } else {
                    sctx->searchsuccessful = 2;

                }
            } else {
                sctx->searchsuccessful = 0;
            }
        }        
        
        hctxEntryOverviewReloadList( ctx );
        //hctxEntryOverviewLoadNames( ctx );
        ctx->rinf = ANIMATION;
                
    } else if ( sctx->searchsuccessful == 2 ) {
        sctx->searchsuccessful = 0;
        
        pushContext(ctx, MESSAGEBOX);
        CTX_MSG_BOX *mctx;
        mctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);

        mctx->mchoices[0] = MSGB_SEARCHINFO_COMMIT;
        mctx->mchoiceicon[0] = SYSICON_OK;
        mctx->choicecount = 1;
        mctx->sel = 0;

        memcpy(mctx->msgtxt, (char*)(textdata + texte[TEXT_SEARCH_NO_ENTRIES]), strlen( (char*)(textdata + texte[TEXT_SEARCH_NO_ENTRIES]) ) + 1 );  
    } else if ( sctx->searchsuccessful == 3 ) {
        sctx->searchsuccessful = 1;
        
        pushContext(ctx, MESSAGEBOX);
        CTX_MSG_BOX *mctx;
        mctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);

        mctx->mchoices[0] = MSGB_SEARCHINFO_COMMIT;
        mctx->mchoiceicon[0] = SYSICON_OK;
        mctx->choicecount = 1;
        mctx->sel = 0;

        memcpy(mctx->msgtxt, (char*)(textdata + texte[TEXT_SEARCH_LAST_ENTRY]), strlen( (char*)(textdata + texte[TEXT_SEARCH_LAST_ENTRY]) ) + 1 );        
    } else if ( isButtonPressed(BUTTON_A) ) {
        
        if ( sctx->cursor < sctx->entrycount ) {
            TCHAR path[16];
            uint16_t cursor = sctx->cursor;
                        
            memcpy( path, &"KS/", 3 );                    
            memcpy( path+3, sctx->entries[cursor - sctx->bufferstart].path, strlen( sctx->entries[cursor - sctx->bufferstart].path ) + 1 );
                        
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
        
        uint8_t s=0;
        
        cbctx->textid[s] = TEXT_CHB_SEARCH_ENTRY;
        cbctx->selectid[s] = CHBX_SEARCH_ENTRY;
        s++;        
        
        cbctx->textid[s] = TEXT_CHB_CREATE_ENTRY;
        cbctx->selectid[s] = CHBX_CREATE_ENTRY;
        s++;
        
        if ( candelete ) {
            cbctx->textid[s] = TEXT_CHB_DELETE_ENTRY;
            cbctx->selectid[s] = CHBX_DELETE_ENTRY;                       
            s++;
        }
        
        cbctx->textid[s] = TEXT_CHB_OPTIONS;
        cbctx->selectid[s] = CHBX_OPTIONS;
        s++;
        
        
        cbctx->textid[s] = TEXT_CHB_GAMES;
        cbctx->selectid[s] = CHBX_GAMES;            
        s++;       
        
        cbctx->options = s;
    } else if ( isButtonPressed(BUTTON_UP) || isButtonPressed(BUTTON_DOWN) ) {
        if ( isButtonPressed(BUTTON_UP) && sctx->cursor > 0 ) sctx->cursor--;
        if ( isButtonPressed(BUTTON_DOWN) && ( sctx->cursor + 1 )  < sctx->entrycount ) sctx->cursor++;
        
        uint16_t start = sctx->cursor - ( OVERVIEW_LINES - 1 ) / 2;
        uint16_t end = start + ( OVERVIEW_LINES - 1 );
        
        if ( start < sctx->bufferstart || end >= sctx->bufferstart + sctx->bufferlen ) {                
            hctxEntryOverviewReloadList( ctx );
            //hctxEntryOverviewLoadNames( ctx );
        }
        
        ctx->rinf = ANIMATION;
    } else if ( isButtonPressed(BUTTON_RIGHT) && sctx->searchsuccessful == 1 ) {        
                  
        uint16_t from = sctx->cursor;
        uint16_t next = searchNextEntryFrom( sctx, from );
        if ( next != -1 ) {
            sctx->cursor = next;
            
            hctxEntryOverviewReloadList( ctx );
            //hctxEntryOverviewLoadNames( ctx );
            ctx->rinf = ANIMATION;        
            
            if ( next <= from ) sctx->searchsuccessful = 3;
        }
    }            
}

void hctxEntryDetail(APP_CONTEXT* ctx) {
    CTX_ENTRY_DETAIL *sctx;
    sctx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);

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
    } else if ( isButtonPressed(BUTTON_A) ) {              
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
        
    } else if ( isButtonPressed(BUTTON_B) ) {                      
        pushContext(ctx, CHOOSEBOX );
                        
        CTX_CHOOSE_BOX *cbctx;
        cbctx = (CTX_CHOOSE_BOX*) (ctx->ctxbuffer + ctx->ctxptr);        

        cbctx->options = 0;
        cbctx->textid[cbctx->options] = TEXT_CHB_RETURN_OVERVIEW;
        cbctx->selectid[cbctx->options] = CHBX_RETURN_OVERVIEW;
        cbctx->options++;        
        
        if ( sctx->tokeninfo[ sctx->selected-1 ].isset && 
             device_options.umode == USB_MODE_KEYBOARD && 
             USBGetDeviceState() == CONFIGURED_STATE ) {
            cbctx->textid[cbctx->options] = TEXT_CHB_PUSH_TOKEN;
            cbctx->selectid[cbctx->options] = CHBX_PUSH_TOKEN;                       
            cbctx->options++;
        }
        
        cbctx->textid[cbctx->options] = TEXT_CHB_EDIT_TOKEN;
        cbctx->selectid[cbctx->options] = CHBX_EDIT_TOKEN;
        cbctx->options++;
        
        if ( sctx->tokeninfo[ sctx->selected-1 ].isset && sctx->selected != NAME ) {        
            cbctx->textid[cbctx->options] = TEXT_CHB_DELETE_TOKEN;
            cbctx->selectid[cbctx->options] = CHBX_DELETE_TOKEN;
            cbctx->options++;
        }
                                      
    }
}


void hctxUSBPush(APP_CONTEXT* ctx) {
    CTX_USB_PUSH *sctx;
    sctx = (CTX_USB_PUSH*) (ctx->ctxbuffer + ctx->ctxptr);
    
    if ( !USB_IsWriteCharBuffer() ) {
        removeContext(ctx);       
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
    } else if ( isButtonPressed(BUTTON_B) ) {
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
            FRESULT fres = createNewEntry(sctx->path);
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
}



void hctxSearchEntries(APP_CONTEXT* ctx) {
    CTX_SEARCH_ENTRIES *sctx;
    sctx = (CTX_SEARCH_ENTRIES*) (ctx->ctxbuffer + ctx->ctxptr);
    
    HCTX_IO_RESULT res = hctxIO( &sctx->io, ctx );
        
    if ( res == IOR_UNCHANGED ) {
        ctx->rinf = UNCHANGED;
    } else if ( res == IOR_UPDATED ) {
        ctx->rinf = IO_UPDATE;     
    } else if ( res == IOR_OK ) {
        uint8_t searchfield[MAX_SEARCH_LEN];
        uint16_t len = sctx->io.tlen;
        memcpy( searchfield, sctx->io.text, len);
        setContext(ctx, ENTRY_OVERVIEW);
        
        CTX_ENTRY_OVERVIEW *evctx;
        evctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);
        
        memcpy( evctx->searchstr, searchfield, len); 
        evctx->needinitsearch = true;
    }
      
}


void hctxMessagebox(APP_CONTEXT* ctx) {
    CTX_MSG_BOX *sctx;
    sctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);
    
    if ( isButtonPressed(BUTTON_LEFT) ) {
        sctx->osel = sctx->sel;        
        sctx->sel = sctx->sel > 0 ? sctx->sel - 1 : 0;    
        if ( sctx->osel != sctx->sel ) ctx->rinf = ANIMATION;
    } else if ( isButtonPressed(BUTTON_RIGHT) ) {
        sctx->osel = sctx->sel;        
        sctx->sel = sctx->sel < sctx->choicecount - 1 ? sctx->sel + 1 : sctx->choicecount - 1;    
        if ( sctx->osel != sctx->sel ) ctx->rinf = ANIMATION;
    } else if ( isButtonPressed(BUTTON_A) ) {
        MSGB_CHOICES result = sctx->mchoices[ sctx->sel ];
        if ( result == MSGB_EDIT_ABORT_YES || result == MSGB_EDIT_ABORT_NO || result == MSGB_EDIT_ACCEPT_YES || result == MSGB_EDIT_ACCEPT_NO ) {
            removeContext(ctx);
            CTX_EDIT_ENTRY *prevctx;
            prevctx = (CTX_EDIT_ENTRY*) (ctx->ctxbuffer + ctx->ctxptr);                
            prevctx->io.mboxresult = result;
            prevctx->io.rinf = REFRESH;
            ctx->rinf = IO_UPDATE;
        } else if ( result == MSGB_DELETE_ENTRY_YES || result == MSGB_DELETE_ENTRY_NO ) {
            removeContext(ctx);
            
            if ( result == MSGB_DELETE_ENTRY_YES ) {
                removeContext(ctx);
                
                CTX_ENTRY_OVERVIEW *prevctx;
                prevctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);                
                        
                //Delete entry            
                TCHAR path[16];
                memcpy(path, &"KS/", 3);                
                memcpy(path+3, prevctx->entries[prevctx->cursor - prevctx->bufferstart].path, strlen(prevctx->entries[prevctx->cursor - prevctx->bufferstart].path) + 1 );

                if ( deleteEntry( path ) != FR_OK ) {
                    setContext( ctx, ERROR_SD_FAILURE  );
                } else {
                    prevctx->initialized = false;
                }                            
            }    
        } else if ( result == MSGB_DELETE_TOKEN_YES || result == MSGB_DELETE_TOKEN_NO ) {
            removeContext(ctx);
            
            CTX_ENTRY_DETAIL *prevctx;
            prevctx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);                
            
            if ( result == MSGB_DELETE_TOKEN_YES ) {                              
                uint8_t emptytext[0];
                modifyEntry( prevctx->path, emptytext, 0, prevctx->selected );
                                
                if ( loadEntryDetail( ctx ) != FR_OK ) {
                    setContext( ctx, ERROR_SD_FAILURE  );
                }
            }            
        } else if ( result == MSGB_MKEY_ERROR_OK ) {
            removeContext(ctx);
            
            CTX_KEY_INPUT *prevctx;
            prevctx = (CTX_KEY_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);
            prevctx->io.rinf = REFRESH;            
            ctx->rinf = IO_UPDATE;
            
        } else if ( result == MSGB_SEARCHINFO_COMMIT ) {
            removeContext(ctx);                      
            ctx->rinf = REFRESH;
        }
                               
    }   
}

void hctxIOSetStartValuesAfterLoad(IO_CONTEXT* ioctx) {
    
    uint16_t currline, currlineoffs, prevlineoffs;
    uint8_t posline, poslinex, currlinelen;
               
    ioctx->tewp = ioctx->tlen;  
    ioctx->rinf = REFRESH;
    
    if ( ioctx->tlen > 1 ) {
        hctxIOGetCurrCursorPosition(ioctx, &currline, &posline, &poslinex, &currlinelen, &currlineoffs, &prevlineoffs );
        if ( currline > 0 ) {
            uint8_t loff = currline;
            if ( loff >= ( TEXTAREA_VIEWPORT_LINES - 1 ) ) loff = TEXTAREA_VIEWPORT_LINES - 1;
            for (int i=0;i<loff;i++) {
                ioctx->tewp -= currlinelen;                        
                hctxIOGetCurrCursorPosition(ioctx, &currline, &posline, &poslinex, &currlinelen, &currlineoffs, &prevlineoffs );
            }                 
            ioctx->tewp = ioctx->tlen;
        }

        ioctx->tavcloff = currline;
        hctxIORecalcTavccoff( ioctx );    
    } else {
        ioctx->tavcloff = 0;
    }
    
}

void hctxChoosebox(APP_CONTEXT* ctx) {
    FRESULT res;
    
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
        
        if ( sel == CHBX_SEARCH_ENTRY ) {
            removeContext(ctx);
            
            CTX_ENTRY_OVERVIEW *ovctx;
            ovctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);
            
            uint8_t searchstr[MAX_SEARCH_LEN];                   
            memcpy(searchstr, ovctx->searchstr, MAX_SEARCH_LEN);
                        
            if ( setContext(ctx, SEARCH_ENTRIES ) ) {
                CTX_SEARCH_ENTRIES *sectx;
                sectx = (CTX_SEARCH_ENTRIES*) (ctx->ctxbuffer + ctx->ctxptr);              
        
                sectx->io.inp = SEARCHFIELD;
                sectx->io.type = NEW;
                sectx->io.kbmap = DEFAULT_KEYMAP;
                ctx->rinf = IO_UPDATE;
                
                sectx->io.text[0] = CHAR_LINEFEED;            
                sectx->io.tlen = strlen( (char *)searchstr ) + 1;
                memcpy(sectx->io.text+1, searchstr, sectx->io.tlen);

                hctxIOSetStartValuesAfterLoad(&sectx->io);
            }          

        } else if ( sel == CHBX_CREATE_ENTRY ) {
            removeContext(ctx);
            
            CTX_ENTRY_OVERVIEW *ovctx;
            ovctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);            
            uint16_t cursor = ovctx->cursor;
            
            if ( replaceContext(ctx, EDIT_ENTRY ) ) {
                CTX_EDIT_ENTRY *ectx;
                ectx = (CTX_EDIT_ENTRY*) (ctx->ctxbuffer + ctx->ctxptr);
                ectx->io.inp = TOKEN; 
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
            //removeContext(ctx);
            
            pushContext(ctx, MESSAGEBOX);
            CTX_MSG_BOX *mctx;
            mctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);

            mctx->mchoices[0] = MSGB_DELETE_ENTRY_YES;
            mctx->mchoiceicon[0] = SYSICON_OK;
            mctx->mchoices[1] = MSGB_DELETE_ENTRY_NO;
            mctx->mchoiceicon[1] = SYSICON_ABORT;
            mctx->choicecount = 2;
            mctx->sel = 0;

            memcpy(mctx->msgtxt, (char*)(textdata + texte[TEXT_MSG_DELETE_ENTRY]), strlen( (char*)(textdata + texte[TEXT_MSG_DELETE_ENTRY]) ) + 1 );
            
        } else if ( sel == CHBX_OPTIONS ) {            
            pushContext(ctx, CHOOSEBOX );

            CTX_CHOOSE_BOX *cbctx;
            cbctx = (CTX_CHOOSE_BOX*) (ctx->ctxbuffer + ctx->ctxptr);        

            uint8_t s=0;

            cbctx->textid[s] = TEXT_CHB_DEVICEOPTIONS;
            cbctx->selectid[s] = CHBX_DEVICEOPTIONS;
            s++;

            cbctx->textid[s] = TEXT_CHB_PINOPTIONS;
            cbctx->selectid[s] = CHBX_PINOPTIONS;            
            s++; 

            cbctx->options = s;
        } else if ( sel == CHBX_DEVICEOPTIONS ) {
            setContext( ctx, DEVICEOPTIONS );   
        } else if ( sel == CHBX_PINOPTIONS ) {
            setContext( ctx, PINOPTIONS ); 
            
            CTX_PINOPTIONS *poctx;
            poctx = (CTX_PINOPTIONS*) (ctx->ctxbuffer + ctx->ctxptr);
            
            poctx->pin_len = device_options.pin_len;
        } else if ( sel == CHBX_GAMES ) {
            setContext( ctx, GAME_TETRIS ); 
            
            CTX_TETRIS *tetctx;
            tetctx = (CTX_TETRIS*) (ctx->ctxbuffer + ctx->ctxptr);
            
            tet_initialize( tetctx );                        
        } else if ( sel == CHBX_RETURN_OVERVIEW ) {
            removeContext(ctx);

            CTX_ENTRY_DETAIL *edctx;
            edctx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);
                        
            uint16_t cursor = edctx->overviewcursor;

            setContext( ctx, ENTRY_OVERVIEW );
            CTX_ENTRY_OVERVIEW *ovctx;
            ovctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr); 
            ovctx->cursor = cursor;                                 
        } else if ( sel == CHBX_EDIT_TOKEN ) {
            TCHAR path[16];
            TOKEN_TYPE currtoken;

            
            removeContext(ctx);
            
            CTX_ENTRY_DETAIL *dectx;
            dectx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);
            
            
            if ( dectx->selected != NEW ) {
                memcpy( path, dectx->path, 16 );
                currtoken = dectx->selected;

                replaceContext(ctx, EDIT_ENTRY ); 

                CTX_EDIT_ENTRY *edctx;
                edctx = (CTX_EDIT_ENTRY*) (ctx->ctxbuffer + ctx->ctxptr); 

                memcpy( edctx->path, path, 16 ); 
                edctx->io.inp = TOKEN;
                edctx->io.type = currtoken;
                edctx->io.text[0] = CHAR_LINEFEED;  
                edctx->io.kbmap = DEFAULT_KEYMAP;
                ctx->rinf = IO_UPDATE;
  
                res = readEntryToken( edctx->path, edctx->io.type, edctx->io.text+1, MAX_TEXT_LEN, &edctx->io.tlen );
                if ( res == FR_OK ) {   
                    hctxIOSetStartValuesAfterLoad(&edctx->io);
                                    
                } else {
                    setContext( ctx, ERROR_SD_FAILURE  );
                }                        
            }            
  
        } else if ( sel == CHBX_DELETE_TOKEN ) {
            removeContext(ctx);
            
            pushContext(ctx, MESSAGEBOX);
            CTX_MSG_BOX *mctx;
            mctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);

            mctx->mchoices[0] = MSGB_DELETE_TOKEN_YES;
            mctx->mchoiceicon[0] = SYSICON_OK;
            mctx->mchoices[1] = MSGB_DELETE_TOKEN_NO;
            mctx->mchoiceicon[1] = SYSICON_ABORT;
            mctx->choicecount = 2;
            mctx->sel = 0;

            memcpy(mctx->msgtxt, (char*)(textdata + texte[TEXT_MSG_DELETE_TOKEN]), strlen( (char*)(textdata + texte[TEXT_MSG_DELETE_TOKEN]) ) + 1 );                        
        } else if ( sel == CHBX_PUSH_TOKEN ) {
            removeContext(ctx);
                            
            if ( USBGetDeviceState() == CONFIGURED_STATE ) {
                CTX_ENTRY_DETAIL *edctx;
                edctx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);                    
                                
                pushContext(ctx, USB_PUSH);

                CTX_USB_PUSH *upctx;
                upctx = (CTX_USB_PUSH*) (ctx->ctxbuffer + ctx->ctxptr);                    
                
                res = readEntryToken( edctx->path, edctx->selected, upctx->text, MAX_TEXT_LEN, &upctx->tlen ); 
                if ( res == FR_OK ) {
                    USB_WriteCharacterBuffer(upctx->text, upctx->tlen);
                }
            }
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


void hctxDeviceOptions(APP_CONTEXT* ctx) {
    CTX_DEVICEOPTIONS *sctx;
    sctx = (CTX_DEVICEOPTIONS*) (ctx->ctxbuffer + ctx->ctxptr);    

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
        } else if ( sctx->selected == 4 ) {
            sctx->selected = 8;
        } else if ( sctx->selected >= 8 ) {
            if ( sctx->selected < 10 ) {
                sctx->selected++;
            }            
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
        } else if ( sctx->selected == 8 ) {
            sctx->selected = 4;
        } else if ( sctx->selected > 8 ) {
            sctx->selected --;
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
        if ( sctx->selected >= 8 ) {
        
            if ( sctx->selected == 8 ) {
                device_options.umode = device_options.umode == USB_MODE_OFF ? USB_MODE_KEYBOARD : USB_MODE_OFF;
            } else if ( sctx->selected == 9 ) {
                device_options.brightness = device_options.brightness > 10 ? device_options.brightness - 10 : device_options.brightness;
                dispSetBrightness( device_options.brightness );
            } else if ( sctx->selected == 10 ) {
                uint8_t c = hctxGetColor();
                if ( c > 0 ) { 
                    hctxSetColor(c-1); 
                } else { 
                    hctxSetColor(sizeof(highlight_color_tab)/sizeof(uint16_t) - 1); 
                };
            } 
                        
        } else if ( sctx->selected != 0 && sctx->selected != 4 ) sctx->selected --;
    } else if ( isButtonPressed( BUTTON_RIGHT ) ) {
        sctx->oselected = sctx->selected;
        if ( sctx->selected >= 8 ) {
        
            if ( sctx->selected == 8 ) {
                device_options.umode = device_options.umode == USB_MODE_OFF ? USB_MODE_KEYBOARD : USB_MODE_OFF;
            } else if ( sctx->selected == 9 ) {
                device_options.brightness = device_options.brightness < 100 ? device_options.brightness + 10 : device_options.brightness;
                dispSetBrightness( device_options.brightness );
            } else if ( sctx->selected == 10 ) {
                uint8_t c = hctxGetColor();
                if ( c < sizeof(highlight_color_tab)/sizeof(uint16_t) - 1 ) { 
                    hctxSetColor(c + 1);
                } else {
                    hctxSetColor(0);
                }
            }   
            
        } else if ( sctx->selected != 3 && sctx->selected != 7 ) sctx->selected ++;  
    } else if ( isButtonPressed( BUTTON_B ) ) {
        replaceContext(ctx, ENTRY_OVERVIEW );        
    }
}

void hctxPinOptions(APP_CONTEXT* ctx) {
    CTX_PINOPTIONS *sctx;
    sctx = (CTX_PINOPTIONS*) (ctx->ctxbuffer + ctx->ctxptr);    
      
    ctx->rinf = ANIMATION;
    sctx->oselected = sctx->selected;
    if ( isButtonPressed(BUTTON_DOWN) ) {
        if ( sctx->selected < 2 ) sctx->selected ++;                
    } else if ( isButtonPressed(BUTTON_UP) ) {
        if ( sctx->selected > 0 ) sctx->selected --;                
    } else if ( isButtonPressed(BUTTON_A) ) { 
        if ( sctx->selected == 0 ) {
            device_options.pin_len = sctx->pin_len;

            pushContext(ctx, PIN_INPUT);

            CTX_PIN_INPUT *pctx;
            pctx = (CTX_PIN_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);
            pctx->generatenew = 1;
            initPinInput( ctx );        
        }
    } else if ( isButtonPressed(BUTTON_RIGHT) ) {
        if ( sctx->selected == 1 ) {
            if ( device_options.pin_tries < MAX_PIN_TRIES ) device_options.pin_tries ++;
        } else if ( sctx->selected == 2 ) {
            if ( sctx->pin_len < MAX_PIN_SIZE ) sctx->pin_len ++;
        }
    } else if ( isButtonPressed(BUTTON_LEFT) ) {
        if ( sctx->selected == 1 ) {
            if ( device_options.pin_tries > MIN_PIN_TRIES ) device_options.pin_tries --;
        } else if ( sctx->selected == 2 ) {
            if ( sctx->pin_len > MIN_PIN_SIZE ) sctx->pin_len --;
        }
    } else if ( isButtonPressed( BUTTON_B ) ) {
        replaceContext(ctx, ENTRY_OVERVIEW );       
    }
}

void loadPinOptions( APP_CONTEXT* ctx ) {
    CTX_PINOPTIONS *sctx;
    sctx = (CTX_PINOPTIONS*) (ctx->ctxbuffer + ctx->ctxptr);    
    
    sctx->pin_len = device_options.pin_len;    
}

void updateDeviceStatus( APP_CONTEXT* ctx ) {
    
    if ( !USB_BUS_SENSE ) {
        ctx->dcstate = DEVCST_BATTERY;                    
        if ( ctx->power > 100 ) ctx->power = 100;        
        
        if ( ctx->adc_rw_state == 0 && !adcTimer ) {                        
            AD1CON1bits.SAMP = 0;
            enableVoltPower();
            ctx->adc_rw_state = 1;
            adcTimer = 2;            
        } else if ( ctx->adc_rw_state == 1 && !adcTimer ) {
            AD1CON1bits.SAMP = 1;
            ctx->adc_rw_state = 2;
        } else if ( ctx->adc_rw_state == 2 ) {
            
            if ( AD1CON1bits.DONE ) {
                int analogValue = ADC1BUF0;
                
                uint8_t p = calculateBatteryPowerFromADC( analogValue );
                if ( p < ctx->power ) ctx->power = p;
                ctx->adc_rw_state = 0;
                disableVoltPower();
                adcTimer = 200;
            }
        }
    } else {
        if ( USBGetDeviceState() == CONFIGURED_STATE ){
            ctx->dcstate = DEVCST_USB;
        } else {
            ctx->dcstate = DEVCST_CHARGER;
        }
                               
        if ( ctx->adc_rw_state == 1 ) {
            AD1CON1bits.SAMP = 0;
            ctx->adc_rw_state = 0;
            adcTimer = 0;
            disableVoltPower();
        } 
        
        ctx->power = 0xff;
    }        
}


void updateContext(APP_CONTEXT* ctx) {            
    if (SENSE_CASE_GetValue() || ctx->power <= SHUTDOWN_POWER_LIMIT ) {
        spi1_open(DISPLAY_CONFIG);
        clearScreen(COLOR_BLACK);
        spi1_close();
        setSleep(ctx);
    }

    spi_fat_open(); //Open FAT SPI while handle logic 
    
    if ( USB_BUS_SENSE ) {
        if ( ctx->bus_sense == 0 ) {
            set_USB_forbid_charge( false );            
            ctx->bus_sense = 1;
            busChargeStartTimer = 2500;
        } else if ( ctx->bus_sense == 1 && !busChargeStartTimer ) {
            ctx->bus_sense = 2;
            if ( !get_USB_forbid_charge() ) {
                enableBUSCharge();
            }
        }
    } else {
        ctx->bus_sense = 0;
        disableBUSCharge();
    }
    
    
    if ( device_options.umode == USB_MODE_KEYBOARD && 
         USB_BUS_SENSE &&
         ctx->ctxtype != INITIAL && 
         ctx->ctxtype != PIN_INPUT && 
         ctx->ctxtype != KEY_INPUT ) {
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
            unmountFS( ctx );
            setContext(ctx, ERROR_SD_CD);
        }        
    } else {
        //SD-Card inserted
        if ( ctx->ctxtype == ERROR_SD_CD ) {            
            setInitialContext( ctx );
        } else {                
            if (!ctx->fsmounted) {
                if ( ctx->ctxtype != INITIAL && 
                     ctx->ctxtype != PIN_INPUT ) {
                    mountFS( ctx );
                    if ( !ctx->fsmounted ) {                        
                        setContext(ctx, ERROR_SD_FAILURE);
                    }
                }
            }
        }
    }
       
    updateDeviceStatus(ctx);
    updateButtons(false);
        
    switch (ctx->ctxtype) {
        case INITIAL:
        {
            
//            setMasterTestKey( );
            if ( isKeySet() ) {
                setContext(ctx, PIN_INPUT);
                initPinInput( ctx );
//                testPinGen( ctx);
            } else {
                setContext(ctx, KEY_INPUT);
                initKeyInput( ctx );
            }
                    

//            setContext(ctx, ENTRY_OVERVIEW);
        }
            break;

        case KEY_INPUT:        
            hctxKeyInput(ctx);
            break;
        case PIN_INPUT:
            hctxPinInput(ctx);
            break;
        case VERIFY_KEY_AFTER_PIN:
            hctxVerifyKeyAfterPin(ctx);
            break;            
        case ENTRY_OVERVIEW:
            hctxEntryOverview(ctx);
            break;
        case ENTRY_DETAIL:
            hctxEntryDetail(ctx);
            break;
        case USB_PUSH:
            hctxUSBPush(ctx);
            break;
        case VIEW_TOKEN:
            hctxViewToken(ctx);
            break;
        case EDIT_ENTRY:
            hctxEditEntry(ctx);        
            break;            
        case SEARCH_ENTRIES:
            hctxSearchEntries(ctx);
            break;
        case MESSAGEBOX:            
            hctxMessagebox(ctx);        
            break;      
        case CHOOSEBOX:
            hctxChoosebox(ctx);
            break;
        case DEVICEOPTIONS:
            hctxDeviceOptions(ctx);        
            break;  
        case PINOPTIONS:
            hctxPinOptions(ctx);        
            break;
        case GAME_TETRIS:
            hctxTetris(ctx);
            break;
    }
    
    spi_fat_close(); //Close FAT SPI while handle logic               
}
