/*
 * File:   ui.c
 * Author: greengravity
 *
 * Created on 20. Januar 2023, 16:08
 */


#include "xc.h"
#include "string.h"
#include "logic.h"
#include "display_driver.h"
#include "assets.h"
#include "mcc_generated_files/spi1_driver.h"


void rndDrawKeylayoutContent( Keylayout *k, uint8_t x, uint8_t y, uint8_t yoff ) {
    
    if ( k->fkt == KEYCOMMAND_CHAR ) {                    
        GFXChar ch = gfxchars[ k->id ];
        int8_t xoff = ( ( ( ( k->span_pos + 1 ) * 16 ) - ch.xadv ) / 2 ) + 1;
        if ( xoff < 0 ) xoff = 0;
        locate( x * 16 + xoff, yoff+ y*16+1 );
        writeChars(&ch, 1);
    } else {
        uint16_t icon = 0; 
        if ( k->fkt == KEYCOMMAND_SPACE ) {
            icon = SYSICON_SPACE;            
        } else if ( k->fkt == KEYCOMMAND_DEL ) {
            icon = SYSICON_DELETE;
        } else if ( k->fkt == KEYCOMMAND_OK ) {
            icon = SYSICON_OK;
        } else if ( k->fkt == KEYCOMMAND_BACK ) {
            //no icon        
        } else if ( k->fkt == KEYCOMMAND_LF ) {
            icon = SYSICON_ENTER;
        }    
        
        if ( icon ) {
            int8_t xoff = ( ( ( ( k->span_pos + 1 ) * 16 ) - bitmaps[icon].width ) / 2 ) + 1;
            if ( xoff < 0 ) xoff = 0;
            
            drawImage( x * 16 + xoff, yoff+ y*16+1, &bitmaps[icon] );
        }
    }             
}


void rndIOKBMap( Keyboardmaps* kmap ) {    
    int16_t yoff = (DISPLAY_HEIGHT - 16 * 4) - 1;    
            
    for (uint8_t y=0;y<5;y++ ) {
        drawFastHLine(0,yoff+y*16, DISPLAY_WIDTH, COLOR_GRAY);
    }
    
    for (uint8_t y=0;y<4;y++) {
        for (uint8_t x=0;x<10;x++) {
            Keylayout *k = (Keylayout *)&keylayouts[y*10+x + kmap->layoutoff];
            if ( k->span_neg == 0 ) {
                rndDrawKeylayoutContent(k, x, y, yoff );                         
                drawFastVLine(( x + k->span_pos + 1 ) * 16,yoff+y*16, 16, COLOR_GRAY);                
            }
        }           
    }
    
    drawFastVLine(0, yoff, 64, COLOR_GRAY);
}

void highlightKeyboardKey( uint8_t x, uint8_t y, Keyboardmaps* km, uint16_t color ) {
    int16_t yoff = ( (DISPLAY_HEIGHT - 16 * 4) - 1 ) + ( y * 16 );    
    Keylayout *kl = (Keylayout *)&keylayouts[y*10+x + km->layoutoff];
    
    uint8_t xoff = ( x - kl->span_neg ) * 16; 
    drawRect( xoff, yoff, 16 * ( kl->span_neg + kl->span_pos + 1 ), 16, color );
} 

void highlightPatternmapKey( uint8_t x, uint8_t y, uint16_t color ) {
    int xoff = 8 + x * 48;
    int yoff = (DISPLAY_HEIGHT - 57) + y * 16;
    drawRect( xoff, yoff, 48, 16, color );
} 


void rndIO(IO_CONTEXT *ioctx) {
    Keyboardmaps* km = (Keyboardmaps*)&keymaps[ ioctx->kbmap ];
    switch ( ioctx->rinf ) {
        case REFRESH:
            clearScreen(COLOR_BLACK);
            
            locate(0,0);               
            cWriteTextIntern((const uint8_t*) (textdata + texte[ token_configs[ioctx->type].textid ]));
            drawFastHLine(0,16, DISPLAY_WIDTH, COLOR_GRAY);
            locate(0,17);
            cWriteTextIntern( (const uint8_t*) (textdata + texte[ TEXT_TEST ]) );
            locate(0,32);
            cWriteTextIntern( (const uint8_t*) (textdata + texte[ TEXT_TEST ]) );
            locate(0,47);
            cWriteTextIntern( (const uint8_t*) (textdata + texte[ TEXT_TEST ]) );
                       
            rndIOKBMap( km );
            if ( ioctx->selarea == 0 ) {
                highlightKeyboardKey( ioctx->kbx, ioctx->kby, km, COLOR_BLUE );
            }
            break;
        case ANIMATION:
            if ( ioctx->selarea == 0 ) {
                highlightKeyboardKey( ioctx->okbx, ioctx->okby, km, COLOR_GRAY );                
                highlightKeyboardKey( ioctx->kbx,  ioctx->kby, km, COLOR_BLUE );                
            } else if (ioctx->selarea == 2 ) { 
                highlightPatternmapKey(ioctx->okbmap % 3 ,ioctx->okbmap / 3, COLOR_GRAY);
                highlightPatternmapKey(ioctx->kbmap % 3 ,ioctx->kbmap / 3, COLOR_BLUE);
            }
            break;
        case AREASWITCH:
            if ( ioctx->selarea == 0 ) {
                if ( ioctx->oselarea == 2 ) {
                    //switched to keyboardarea from keyboardmap
                    fillRect( 0, (DISPLAY_HEIGHT - 16 * 4) - 1, DISPLAY_WIDTH, 16*4, COLOR_BLACK );
                    rndIOKBMap( km );
                    highlightKeyboardKey( ioctx->kbx, ioctx->kby, km, COLOR_BLUE );                    
                } else if ( ioctx->oselarea == 1 ) {
                    //switched to keyboardarea from textarea
                    drawFastHLine(0,16, DISPLAY_WIDTH, COLOR_GRAY);
                    drawFastHLine(0,(DISPLAY_HEIGHT - 16 * 4) - 1, DISPLAY_WIDTH, COLOR_GRAY);
                    highlightKeyboardKey( ioctx->kbx,  ioctx->kby, km, COLOR_BLUE );                                    
                }
                
            } else if ( ioctx->selarea == 1 ) {
                //switched to textarea
                highlightKeyboardKey( ioctx->kbx, ioctx->kby, km, COLOR_GRAY );     
                drawFastHLine(0,16, DISPLAY_WIDTH, COLOR_BLUE);
                drawFastHLine(0,(DISPLAY_HEIGHT - 16 * 4) - 1, DISPLAY_WIDTH, COLOR_BLUE);                
            } else if ( ioctx->selarea == 2 ) {
                highlightKeyboardKey( ioctx->kbx, ioctx->kby, km, COLOR_GRAY );
                fillRect(8, (DISPLAY_HEIGHT - 57), 144, 48, COLOR_BLACK );
                for (int x=0;x<3;x++) {
                    for (int y=0;y<3;y++) {
                        int map = y * 3 + x;                        
                        
                        if ( keymaps[map].active ) {
                            int xoff = 8 + x * 48;
                            int yoff = (DISPLAY_HEIGHT - 57) + y * 16;
                            drawRect(xoff, yoff, 48, 16, COLOR_WHITE );


                            int len = strlen( (const char*)&keymaps[map].name );                        
                            int plen = 0;
                            for ( int i=0;i<len;i++ ) {
                                plen += gfxchars[ keymaps[map].name[i] ].xadv;
                            }
                            locate( xoff + ( ( 48 - plen) / 2 ), yoff+1);                        
                            cWriteTextIntern( (const uint8_t *)&keymaps[map].name );                        
                        }                        
                    }
                }
                highlightPatternmapKey(ioctx->kbmap % 3 ,ioctx->kbmap / 3, COLOR_BLUE);
            }
            break;
        default:
            break;
    }
    
    ioctx->rinf = UNCHANGED;
}

void rndKeyInput(APP_CONTEXT* ctx) {
    CTX_KEY_INPUT *sctx;
    sctx = (CTX_KEY_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);

    int16_t xoff = (DISPLAY_WIDTH - 16 * 4) / 2;
    int16_t yoff = (DISPLAY_HEIGHT - 16 * 4) - 1;

    if (sctx->haderror && !sctx->error) {
        //delete error message
        fillRect(0, 15, DISPLAY_WIDTH, 15, COLOR_BLACK);
    }

    switch (ctx->rinf) {
        case REFRESH:
        {
            clearScreen(COLOR_BLACK);
            for (int16_t x = 0; x < 4; x++) {
                for (int16_t y = 0; y < 4; y++) {
                    GFXChar ch = gfxchars[ hexchars[y * 4 + x] ];
                    locate(xoff + 1 + x * 16 + ((16 - ch.xadv) / 2), yoff + 1 + y * 16);
                    writeChars(&ch, 1);
                }
            }

            for (int16_t x = 0; x <= 4; x++) {
                drawFastVLine(xoff + x * 16, yoff, 16 * 4, COLOR_WHITE);
            }
            for (int16_t y = 0; y <= 4; y++) {
                drawFastHLine(xoff, yoff + y * 16, 16 * 4, COLOR_WHITE);
            }

            drawRect(xoff + sctx->x * 16, yoff + sctx->y * 16, 16, 16, COLOR_BLUE);

            locate(0, 0);
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_ENTER_NEW_KEY ]));


            if (sctx->error) {
                locate(0, 15);
                cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_ENTER_NEW_KEY_ERROR ]));
            }

            locate(0, 15);
        }
            break;
        case ANIMATION:
        {
            drawRect(xoff + sctx->oldx * 16, yoff + sctx->oldy * 16, 16, 16, COLOR_WHITE);
            drawRect(xoff + sctx->x * 16, yoff + sctx->y * 16, 16, 16, COLOR_BLUE);
        }
            break;
        case ONBUTTON_PRESS:
        {
            drawRect(xoff + sctx->x * 16, yoff + sctx->y * 16, 16, 16, COLOR_RED);

            sctx->charlocations[(sctx->kpos - 1) * 2] = getLocationX();
            sctx->charlocations[(sctx->kpos - 1) * 2 + 1] = getLocationY();

            GFXChar ch = gfxchars[ hexchars[sctx->newkey[ sctx->kpos - 1 ] ] ];
            writeChars(&ch, 1);
        }
            break;
        case ONBUTTON_RELEASE:
        {
            drawRect(xoff + sctx->x * 16, yoff + sctx->y * 16, 16, 16, COLOR_BLUE);
        }
            break;
        case REMOVECHAR:
        {
            if (sctx->kpos >= 0) {

                uint8_t x = sctx->charlocations[sctx->kpos * 2];
                uint8_t y = sctx->charlocations[sctx->kpos * 2 + 1];

                locate(x, y);
                GFXChar ch = gfxchars[ hexchars[sctx->newkey[ sctx->kpos] ] ];
                unwriteChars(&ch, 1);
                locate(x, y);

            }
        }
        default: break;
    }
}

void rndPinInput(APP_CONTEXT* ctx) {
    CTX_PIN_INPUT *sctx;
    sctx = (CTX_PIN_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);

    if (sctx->generatenew) {
        if (ctx->rinf == REFRESH) {
            clearScreen(COLOR_BLACK);
            if (sctx->error) {
                locate(0, 70);
                cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_ENTER_NEW_PIN_ERROR ]));
            }

            locate(0, 00);
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_ENTER_NEW_PIN1 ]));
            locate(0, 15);
        } else if (ctx->rinf == ANIMATION) {
            locate(0, 35);
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_ENTER_NEW_PIN2 ]));
            locate(0, 50);
        } else if (ctx->rinf == ONBUTTON_PRESS) {
            writeText("*");
            if (!sctx->error && sctx->haderror) {
                //remove error message
                fillRect(0, 70, DISPLAY_WIDTH, 15, COLOR_BLACK);
            }
        }
    } else {
        switch (ctx->rinf) {
            case REFRESH:
            {
                clearScreen(COLOR_BLACK);
                locate(0, 50);
                cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_ENTER_PIN ]));
                locate(0, 56);
            }
                break;
            case ONBUTTON_PRESS:
            {
                writeText("*");
            }
                break;
            default:
                break;
        }
    }

}

void rndEntryOverview(APP_CONTEXT* ctx) {
    CTX_ENTRY_OVERVIEW *sctx;
    sctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);

    if (ctx->rinf == REFRESH) {
        clearScreen(COLOR_BLACK);
        locate(0, 0);
        cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_TEST ]));
                       
    }
}

void rndNewEntry(APP_CONTEXT* ctx) {
    CTX_NEW_ENTRY *sctx;
    sctx = (CTX_NEW_ENTRY*) (ctx->ctxbuffer + ctx->ctxptr);

    if (ctx->rinf == IO_UPDATE) {
        rndIO( &sctx->io );
    }
}

void rndError(APP_CONTEXT* ctx) {
    if ( ctx->rinf == REFRESH ) {
        CTX_ERROR *sctx;
        sctx = (CTX_ERROR*) (ctx->ctxbuffer + ctx->ctxptr);
        writeText( sctx->msg );    
    }
}

void renderUI(APP_CONTEXT* ctx) {
    spi1_open(DISPLAY_CONFIG);

    switch (ctx->ctxtype) {
        case KEY_INPUT:
            rndKeyInput(ctx);
            break;
        case PIN_INPUT:
            rndPinInput(ctx);
            break;
        case ENTRY_OVERVIEW:
            rndEntryOverview(ctx);
            break;
        case NEW_ENTRY:
            rndNewEntry(ctx);
            break;
            
        case ERROR:
            rndError(ctx);
        case ERROR_CONTEXT:
        case ERROR_SD_CD:            
        case ERROR_SD_FAILURE:                        
            if ( ctx->rinf == REFRESH ) {
                clearScreen(COLOR_BLACK);
                locate(0,0);
                if ( ctx->ctxtype == ERROR_CONTEXT ) {
                    cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_ERROR_CONTEXT ]));
                } else if ( ctx->ctxtype == ERROR_SD_CD ) {
                    cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_ERROR_SD_CD ]));                    
                } else if ( ctx->ctxtype == ERROR_SD_FAILURE ) {
                    cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_ERROR_SD_FAILURE ]));
                }                        
            }
            break;
    }

    ctx->rinf = UNCHANGED;

    spi1_close();
}