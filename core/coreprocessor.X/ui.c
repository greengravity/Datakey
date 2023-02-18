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
#include "mcc_generated_files/rtcc.h"
#include "buttons.h"
#include "mcc_generated_files/rtcc.h"


void rndDrawKeylayoutContent( Keylayout *k, uint8_t x, uint8_t y, uint8_t yoff, IO_CONTEXT *ioctx ) {
    
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
        } else if ( k->fkt == KEYCOMMAND_BACKSPACE ) {
            icon = SYSICON_BACKSPACE;
        } else if ( k->fkt == KEYCOMMAND_OK ) {
            icon = SYSICON_OK;
        } else if ( k->fkt == KEYCOMMAND_ABORT ) {
            icon = SYSICON_ABORT;
        } else if ( k->fkt == KEYCOMMAND_LF ) {
            icon = SYSICON_ENTER;
        } else if ( k->fkt == KEYCOMMAND_GEN ) {
        
            int len = strlen( (const char*)( textdata + generatormaps[ k->id ].name ) );
            int plen = 0;
            for ( int i=0;i<len;i++ ) {
                plen += gfxchars[ textdata[ generatormaps[ k->id ].name+i ] ].xadv;
            }

            int8_t xoff = ( ( ( ( k->span_pos + 1 ) * 16 ) - plen ) / 2 ) + 1;
            locate( x * 16 + xoff, yoff+ y*16+1);                                              
            cWriteTextIntern( (const uint8_t *)(textdata + generatormaps[ k->id ].name ) );              
        }
        
        if ( icon ) {
            int8_t xoff = ( ( ( ( k->span_pos + 1 ) * 16 ) - bitmaps[icon].width ) / 2 ) + 1;
            if ( xoff < 0 ) xoff = 0;
            
            drawImage( x * 16 + xoff, yoff+ y*16+1, &bitmaps[icon] );
        }
    }             
}


void rndIOKBMap( Keyboardmaps* kmap, IO_CONTEXT *ioctx ) {    
    int16_t yoff = (DISPLAY_HEIGHT - 16 * 4) - 1;    
            
    for (uint8_t y=0;y<5;y++ ) {
        drawFastHLine(0,yoff+y*16, DISPLAY_WIDTH, COLOR_GRAY);
    }
    
    for (uint8_t y=0;y<4;y++) {
        for (uint8_t x=0;x<10;x++) {
            Keylayout *k = (Keylayout *)&keylayouts[y*10+x + kmap->layoutoff];
            if ( k->span_neg == 0 ) {
                rndDrawKeylayoutContent(k, x, y, yoff, ioctx );                         
                drawFastVLine(( x + k->span_pos + 1 ) * 16,yoff+y*16, 16, COLOR_GRAY);                
            }
        }           
    }
    
    drawFastVLine(0, yoff, 64, COLOR_GRAY);
}

void rndIOTextarea( IO_CONTEXT *ioctx ) {
    uint8_t y = 17;
    uint16_t cchar = ioctx->tavccoff;
    uint8_t clx=0, cly=y;
    
    for ( int i=0;i<TEXTAREA_VIEWPORT_LINES;i++ ) {
        uint8_t x;
        locate(0,y);
                       
        uint8_t cline = getCharactersInLine(ioctx->text, cchar, ioctx->tlen - cchar );
        for ( uint8_t j=0;j<cline;j++) {
           
            writeChar( &gfxchars[ ioctx->text[ j + cchar ] < TOTAL_CHAR_COUNT ? ioctx->text[ j + cchar ] : CHAR_ENCODEERR ] );
            
            if ( ( j + cchar ) == ioctx->tewp - 1 ) {
                clx = getLocationX();
                cly = getLocationY();
                fillRect( clx,cly,1,15, COLOR_BLACK);
                fillRect( clx+1,cly,2,15, COLOR_GRAY);
                fillRect( clx+3,cly,1,15, COLOR_BLACK);
                locate(clx+4,cly);
            };   
        }
        x = getLocationX();              
        fillRect(x, y, ( TEXTAREA_WIDTH - x ) + 3, 15, COLOR_BLACK);
      
        cchar += cline;
        
        y+=15;
    }
         
    if ( ioctx->tavcloff > 0 ) {
        drawImage( DISPLAY_WIDTH-5, 23 , &bitmaps[SYSICON_ARROWUP] );
    } else {
        fillRect( DISPLAY_WIDTH-5, 23, 5,12, COLOR_BLACK );
    }
    
    if ( cchar < ioctx->tlen ) {
        drawImage( DISPLAY_WIDTH-5, 45 , &bitmaps[SYSICON_ARROWDOWN] );        
    } else {
        fillRect( DISPLAY_WIDTH-5, 45, 5,12, COLOR_BLACK );
    }        
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

void rndPatternMap( IO_CONTEXT *ioctx, Keyboardmaps* km ) {
    highlightKeyboardKey( ioctx->kbx, ioctx->kby, km, COLOR_GRAY );
    fillRect(8, (DISPLAY_HEIGHT - 57), 144, 48, COLOR_BLACK );
    for (int x=0;x<3;x++) {
        for (int y=0;y<3;y++) {
            int map = y * 3 + x;                        

            if ( keymaps[map].active ) {
                int xoff = 8 + x * 48;
                int yoff = (DISPLAY_HEIGHT - 57) + y * 16;
                drawRect(xoff, yoff, 48, 16, COLOR_WHITE );

                int len = strlen( (const char*)( textdata + keymaps[map].name ) );                      
                int plen = 0;
                for ( int i=0;i<len;i++ ) {
                    plen += gfxchars[ textdata[ keymaps[map].name+i ] ].xadv;
                }

                locate( xoff + ( ( 48 - plen) / 2 ), yoff+1);                                              
                cWriteTextIntern( (const uint8_t *)(textdata + keymaps[map].name ) );                                                                                                                    
            }                        
        }
    }
    highlightPatternmapKey(ioctx->kbmap % 3 ,ioctx->kbmap / 3, device_options.highlight_color1);    
}

void rndIO(IO_CONTEXT *ioctx) {
    Keyboardmaps* km = (Keyboardmaps*)&keymaps[ ioctx->kbmap ];
    switch ( ioctx->rinf ) {
        case REFRESH:
            clearScreen(COLOR_BLACK);
            
            locate(0,0);               
            cWriteTextIntern((const uint8_t*) (textdata + texte[ token_configs[ioctx->type].textid ]));
            drawFastHLine(0,16, DISPLAY_WIDTH, COLOR_GRAY);
                 
            rndIOTextarea( ioctx );
            
            rndIOKBMap( km, ioctx );
            if ( ioctx->selarea == 0 ) {
                highlightKeyboardKey( ioctx->kbx, ioctx->kby, km, device_options.highlight_color1 );
            }
            if ( ioctx->selarea == 2 ) {
                rndPatternMap( ioctx,  km );
            }
            break;
        case ANIMATION:
            if ( ioctx->selarea == 0 ) {
                highlightKeyboardKey( ioctx->okbx, ioctx->okby, km, COLOR_GRAY );                
                highlightKeyboardKey( ioctx->kbx,  ioctx->kby, km, device_options.highlight_color1 ); 
            } else if ( ioctx->selarea == 1 ) {
                rndIOTextarea( ioctx );
            } else if (ioctx->selarea == 2 ) { 
                highlightPatternmapKey(ioctx->okbmap % 3 ,ioctx->okbmap / 3, COLOR_GRAY);
                highlightPatternmapKey(ioctx->kbmap % 3 ,ioctx->kbmap / 3, device_options.highlight_color1);
            }
            break;
        case AREASWITCH:
            if ( ioctx->selarea == 0 ) {
                if ( ioctx->oselarea == 2 ) {
                    //switched to keyboardarea from keyboardmap
                    fillRect( 0, (DISPLAY_HEIGHT - 16 * 4) - 1, DISPLAY_WIDTH, 16*4, COLOR_BLACK );
                    rndIOKBMap( km, ioctx );
                    highlightKeyboardKey( ioctx->kbx, ioctx->kby, km, device_options.highlight_color1 );                    
                } else if ( ioctx->oselarea == 1 ) {
                    //switched to keyboardarea from textarea
                    drawFastHLine(0,16, DISPLAY_WIDTH, COLOR_GRAY);
                    drawFastHLine(0,(DISPLAY_HEIGHT - 16 * 4) - 1, DISPLAY_WIDTH, COLOR_GRAY);
                    highlightKeyboardKey( ioctx->kbx,  ioctx->kby, km, device_options.highlight_color1 );                                    
                }
                
            } else if ( ioctx->selarea == 1 ) {
                //switched to textarea
                highlightKeyboardKey( ioctx->kbx, ioctx->kby, km, COLOR_GRAY );     
                drawFastHLine(0,16, DISPLAY_WIDTH, device_options.highlight_color1);
                drawFastHLine(0,(DISPLAY_HEIGHT - 16 * 4) - 1, DISPLAY_WIDTH, device_options.highlight_color1);                
            } else if ( ioctx->selarea == 2 ) {
                rndPatternMap( ioctx,  km );
                /*
                highlightKeyboardKey( ioctx->kbx, ioctx->kby, km, COLOR_GRAY );
                fillRect(8, (DISPLAY_HEIGHT - 57), 144, 48, COLOR_BLACK );
                for (int x=0;x<3;x++) {
                    for (int y=0;y<3;y++) {
                        int map = y * 3 + x;                        
                        
                        if ( keymaps[map].active ) {
                            int xoff = 8 + x * 48;
                            int yoff = (DISPLAY_HEIGHT - 57) + y * 16;
                            drawRect(xoff, yoff, 48, 16, COLOR_WHITE );
                            
                            int len = strlen( (const char*)( textdata + keymaps[map].name ) );                      
                            int plen = 0;
                            for ( int i=0;i<len;i++ ) {
                                plen += gfxchars[ textdata[ keymaps[map].name+i ] ].xadv;
                            }
                            
                            locate( xoff + ( ( 48 - plen) / 2 ), yoff+1);                                              
                            cWriteTextIntern( (const uint8_t *)(textdata + keymaps[map].name ) );                                                                                                                    
                        }                        
                    }
                }
                highlightPatternmapKey(ioctx->kbmap % 3 ,ioctx->kbmap / 3, COLOR_BLUE);
                */
            }
            break;
        case ONBUTTON_PRESS:
            rndIOTextarea( ioctx );
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

            drawRect(xoff + sctx->x * 16, yoff + sctx->y * 16, 16, 16, device_options.highlight_color1);

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
            drawRect(xoff + sctx->x * 16, yoff + sctx->y * 16, 16, 16, device_options.highlight_color1);
        }
            break;
        case ONBUTTON_PRESS:
        {
            drawRect(xoff + sctx->x * 16, yoff + sctx->y * 16, 16, 16, device_options.highlight_color2);

            sctx->charlocations[(sctx->kpos - 1) * 2] = getLocationX();
            sctx->charlocations[(sctx->kpos - 1) * 2 + 1] = getLocationY();

            GFXChar ch = gfxchars[ hexchars[sctx->newkey[ sctx->kpos - 1 ] ] ];
            writeChars(&ch, 1);
        }
            break;
        case ONBUTTON_RELEASE:
        {
            drawRect(xoff + sctx->x * 16, yoff + sctx->y * 16, 16, 16, device_options.highlight_color1);
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

void highlightEntryLineMid( uint8_t line, uint16_t color, uint8_t start, uint8_t width ) {
    uint8_t y = 19;
    
    y += line * 17;
    
    drawFastHLine(start,y, width, color);
    y += 17;
    drawFastHLine(start,y, width, color);
}

void highlightEntryLine( uint8_t line, uint16_t color, uint8_t width ) {
    uint8_t y = 19;
    
    y += line * 17;
    
    drawFastHLine(0,y, width, color);
    y += 17;
    drawFastHLine(0,y, width, color);
}


void rndEntryOverview(APP_CONTEXT* ctx) {
    CTX_ENTRY_OVERVIEW *sctx;
    sctx = (CTX_ENTRY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);

    bool drawdown=true;
    bool drawup = true;

    if ( ctx->rinf == REFRESH || ctx->rinf == ANIMATION || ctx->rinf == LOADENTRY ) {
        if (ctx->rinf == REFRESH) {           
            clearScreen(COLOR_BLACK);
            locate(0, 0);
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_HEAD_ENTRY_OVERVIEW ]));
            drawFastHLine(0,16, DISPLAY_WIDTH, COLOR_GRAY);       
        }
         
        if ( sctx->initialized ) {
            int32_t start;
            if ( sctx->entrycount > OVERVIEW_LINES ) {
                start = sctx->cursor;  
                start -= ( OVERVIEW_LINES / 2 - 1 );
                if ( start <= 0 ) {
                    start = 0;
                    drawup = false;
                } else {        
                    int32_t end = sctx->entrycount;  
                    end -= OVERVIEW_LINES;
                    if ( start >= end ) {
                        start = end;
                        drawdown = false;
                    }
                }            
            } else {
                start = 0;
            }        


            uint8_t y = 20;
            uint8_t highlightpos = 0;

            setWritebounds( 0, DISPLAY_WIDTH - 6 );       
            for (uint16_t pos = 0; pos < OVERVIEW_LINES; pos ++ ) {
                locate(0, y);
                if ( start >= sctx->bufferstart && start < sctx->bufferstart + sctx->bufferlen ) {                
                    if ( sctx->entries[start].loaded ) {
                        cWriteTextInternNLB(sctx->entries[start].name );
                    } else {
                        cWriteTextInternNLB( textdata+texte[TEXT_LOAD_ENTRY_PLACEHOLDER] );
                    }                
                }
                uint8_t x = getLocationX();
                fillRect(x,y,(DISPLAY_WIDTH - 5 ) - x, 15, COLOR_BLACK );

                if ( start == sctx->cursor ) highlightpos = pos;                        
                start++;
                y += 17;
            }               

            if ( ctx->rinf == ANIMATION || ctx->rinf == REFRESH ) {
                highlightEntryLine( sctx->ohighlightpos, COLOR_BLACK, DISPLAY_WIDTH );
                sctx->ohighlightpos = highlightpos;
                highlightEntryLine( highlightpos, device_options.highlight_color1, DISPLAY_WIDTH );

            }
            setWritebounds( 0, DISPLAY_WIDTH );

            if ( drawup ) {
                drawImage( DISPLAY_WIDTH-5, 23, &bitmaps[SYSICON_ARROWUP] );
            } else {
                fillRect( DISPLAY_WIDTH-5, 23, 5,12, COLOR_BLACK );
            }

            start = sctx->cursor;
            start-= OVERVIEW_LINES / 2;
            if ( drawdown ) {
                drawImage( DISPLAY_WIDTH-5, 50+60 , &bitmaps[SYSICON_ARROWDOWN] );        
            } else {
                fillRect( DISPLAY_WIDTH-5, 50+60, 5,12, COLOR_BLACK );
            }    
        }
    }
}

void highlightEntryDetail( TOKEN_TYPE selection, uint16_t color ) {
    if  (selection == NEW ) return;
    highlightEntryLine( selection - 1, color, DISPLAY_WIDTH );
}

void rndEntryDetail( APP_CONTEXT* ctx ) {
    CTX_ENTRY_DETAIL *sctx;
    sctx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);
    
    if (ctx->rinf == REFRESH) {
        uint8_t y;
        clearScreen(COLOR_BLACK);
        locate(0, 0);
        cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_HEAD_ENTRY_DETAIL ]));
        drawFastHLine(0,16, DISPLAY_WIDTH, COLOR_GRAY);
        
        y = 20;
        locate(0, y);
        cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_EDETAIL_NAME ]));
        locate(36, y);
        cWriteTextInternNLB(sctx->name);
        y+=17;
        
        locate(0, y);
        cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_EDETAIL_KEY1 ]));
        locate(36, y);
        cWriteTextInternNLB(sctx->key1);
        y+=17;

        locate(0, y);
        cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_EDETAIL_KEY2 ]));
        locate(36, y);
        cWriteTextInternNLB(sctx->key2);
        y+=17;
                
        locate(0, y);
        cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_EDETAIL_URL ]));
        locate(36, y);
        cWriteTextInternNLB(sctx->url);
        y+=17;
        
        locate(0, y);
        cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_EDETAIL_INFO ]));
        locate(36, y);
        cWriteTextInternNLB(sctx->info);
        y+=17;
                
        highlightEntryDetail(sctx->selected, device_options.highlight_color1);
    } else if (ctx->rinf == ANIMATION) {  
        highlightEntryDetail(sctx->oselected, COLOR_BLACK);
        highlightEntryDetail(sctx->selected, device_options.highlight_color1);
    }
}


void rndEditEntry(APP_CONTEXT* ctx) {
    CTX_EDIT_ENTRY *sctx;
    sctx = (CTX_EDIT_ENTRY*) (ctx->ctxbuffer + ctx->ctxptr);

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

void rngMessageBoxOption( MSGB_RESULT result, uint16_t selcolor ) {
    if ( result == MSGB_NO_RESULT ) return;   
            
    if ( result == MSGB_YES ) {
        drawRect( 30, DISPLAY_HEIGHT-47, 30, 17, selcolor );
        drawImage(38, DISPLAY_HEIGHT-45, &bitmaps[SYSICON_OK] );
    }
    if ( result == MSGB_NO ) {
        drawRect( DISPLAY_WIDTH-60, DISPLAY_HEIGHT-47, 30, 17, selcolor );
        drawImage(DISPLAY_WIDTH-52, DISPLAY_HEIGHT-45, &bitmaps[SYSICON_ABORT] );
    }    
}

void rndMessagebox(APP_CONTEXT* ctx) {
    CTX_MSG_BOX *sctx;
    sctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);    
    
    if ( ctx->rinf == REFRESH || ctx->rinf == ANIMATION ) {                
        if ( ctx->rinf == REFRESH ) {
            fillRect( 20,20, DISPLAY_WIDTH-40, DISPLAY_HEIGHT-40, COLOR_BLACK );
            drawRect( 20,20, DISPLAY_WIDTH-40, DISPLAY_HEIGHT-40, COLOR_WHITE );
        }

        setWritebounds( 24, DISPLAY_WIDTH-24 );
        locate( 24, 24 );
        cWriteTextIntern( sctx->msgtxt );
        rngMessageBoxOption( MSGB_YES, sctx->res == MSGB_YES ? device_options.highlight_color1 : COLOR_BLACK );
        rngMessageBoxOption( MSGB_NO, sctx->res == MSGB_NO ? device_options.highlight_color1 : COLOR_BLACK );
        setWritebounds( 0, DISPLAY_WIDTH );
    }
}

void highlightChooseBoxEntry(uint8_t yoff, uint8_t selection, uint16_t color ) {
    fillRect( 14, yoff + 3 + selection * 17, DISPLAY_WIDTH - 28, 1, color );    
    fillRect( 14, yoff + 3 + selection * 17 + 17, DISPLAY_WIDTH - 28,1, color );    
}

void rndChoosebox(APP_CONTEXT* ctx) {
    
    CTX_CHOOSE_BOX *sctx;
    sctx = (CTX_CHOOSE_BOX*) (ctx->ctxbuffer + ctx->ctxptr);    
    
    if ( ctx->rinf == REFRESH || ctx->rinf == ANIMATION ) {  
        uint8_t height = sctx->options * 17 + 6;
        uint8_t yoff = ( DISPLAY_HEIGHT - height ) / 2;
        uint8_t xoff = 10;
        
        if ( ctx->rinf == REFRESH ) {
            fillRect( xoff,yoff, DISPLAY_WIDTH-20, height, COLOR_BLACK );
            drawRect( xoff,yoff, DISPLAY_WIDTH-20, height,  COLOR_WHITE );
        }
        
        setWritebounds( 14, DISPLAY_WIDTH-14 );
        for (uint8_t i =0; i<sctx->options; i++) {
            locate( 14, yoff + 4 + i * 17 );
            cWriteTextIntern( (const uint8_t*) (textdata + texte[ sctx->textid[i] ]) );
        }
        
        highlightChooseBoxEntry(yoff, sctx->oselected, COLOR_BLACK );
        highlightChooseBoxEntry(yoff, sctx->selected, device_options.highlight_color1 );
                 
        setWritebounds( 0, DISPLAY_WIDTH );
    }
    
}

void rndViewToken(APP_CONTEXT* ctx) {
    CTX_VIEW_TOKEN *sctx;
    sctx = (CTX_VIEW_TOKEN*) (ctx->ctxbuffer + ctx->ctxptr);     


    if ( ctx->rinf == REFRESH ) {
        clearScreen(COLOR_BLACK);
        
        locate(0,0);               
        cWriteTextIntern((const uint8_t*) (textdata + texte[ token_configs[sctx->type].textidview ]));
        drawFastHLine(0,16, DISPLAY_WIDTH, COLOR_GRAY);        
    }
    
    if (ctx->rinf == REFRESH || ctx->rinf == ANIMATION ) {
        
        uint16_t currline, len, offs;
        currline = offs = 0;
        while ( true ) {
            if ( currline == sctx->currline ) break;
            currline++;
            len = getCharactersInLine(sctx->text, offs, sctx->tlen-offs );                        
            offs += len;        
        }

        uint8_t y,x;
        y = 18;
        for ( int i=0;i<TEXTVIEWER_LINES;i++ ) {
            locate(0,y);    
            
            len = offs < sctx->tlen ? getCharactersInLine(sctx->text, offs, sctx->tlen - offs ) : 0;                                             
            for ( int j=0;j<len;j++) writeChar( &gfxchars[ sctx->text[ j + offs ] < TOTAL_CHAR_COUNT ? sctx->text[ j + offs ] : CHAR_ENCODEERR ] );
            offs += len;
            x = getLocationX();
            fillRect(x, y, ( TEXTAREA_WIDTH - x ) + 3, 15, COLOR_BLACK);
            y+=17;
        }
    }
           
    if ( sctx->currline > 0 ) {
        drawImage( DISPLAY_WIDTH-5, 23 , &bitmaps[SYSICON_ARROWUP] );
    } else {
        fillRect( DISPLAY_WIDTH-5, 23, 5,12, COLOR_BLACK );
    }
    
    int32_t lines = sctx->lines;
    lines -= TEXTVIEWER_LINES;
    if ( lines < 0 ) lines = 0;
    if ( sctx->currline < lines ) {
        drawImage( DISPLAY_WIDTH-5, 50+60 , &bitmaps[SYSICON_ARROWDOWN] );        
    } else {
        fillRect( DISPLAY_WIDTH-5, 50+60, 5,12, COLOR_BLACK );
    }  
                   
}


void rndOptions1(APP_CONTEXT* ctx) {
    CTX_OPTIONS1 *sctx;
    sctx = (CTX_OPTIONS1*) (ctx->ctxbuffer + ctx->ctxptr);         
    
    if ( ctx->rinf == REFRESH || ctx->rinf == ANIMATION ) {
        uint8_t y;
        uint8_t x;
        
        if ( ctx->rinf == REFRESH ) {
            clearScreen(COLOR_BLACK);
            locate(0, 0);
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_HEAD_OPTIONS1 ]));
            drawFastHLine(0,16, DISPLAY_WIDTH, COLOR_GRAY);
        }
                        
        y = 20;
        drawImage( 5, y, &bitmaps[SYSICON_USB] );
        locate(25, y);
        if ( device_options.umode == USB_MODE_OFF ) {
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_OPTV_USB_OFF ]));
        } else if ( device_options.umode == USB_MODE_KEYBOARD ) {
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_OPTV_USB_KEYBOARD ]));            
        }
        x = getLocationX();
        fillRect(x, y, DISPLAY_WIDTH-x, 15, COLOR_BLACK );
                        
        y+=17;
        drawImage( 5, y, &bitmaps[SYSICON_BRIGHTNESS] );
        locate(25, y);

        fillRect(23, y+1, 2, 13, COLOR_BLACK ); 
        fillRect(25+64, y+1, 2, 13, COLOR_BLACK );
        for (int i=0;i<64;i++) {
            uint16_t gcb;
            uint16_t gradient_color;
                        
            gcb = i*4;
            gradient_color = ( ( gcb & 0xf8 ) << 8 ) | ( ( gcb & 0xfc ) << 3 ) | ( ( gcb & 0xf8 ) >> 3 );
                        
            fillRect(25+i, y+1, 1, 13, gradient_color );            
        }
        int bloc = device_options.brightness;
        bloc = ( bloc * 64 ) / 100;
        fillRect(25+bloc, y+1, 1, 13, device_options.highlight_color1 );
        fillRect(25+bloc-1, y+1, 1, 13, COLOR_BLACK );
        fillRect(25+bloc+1, y+1, 1, 13, COLOR_BLACK );
          
        y+=17;
        drawImage( 5, y, &bitmaps[SYSICON_COLOR] );
        locate(25, y);
        
        fillRect( 25, y+3, 64, 9, device_options.highlight_color1 );
        
        highlightEntryLine(sctx->oselected, COLOR_BLACK, DISPLAY_WIDTH);
        highlightEntryLine(sctx->selected, device_options.highlight_color1, DISPLAY_WIDTH );
    }           
}


void setPositiontab(uint8_t selection,  uint8_t y, uint8_t x1, uint8_t x2, uint32_t *positiontab ) {
    uint32_t v, v2;
    
    
    v2 = 0;
    v = y;    
    v2 |= ( v << 16 );
    v = x1;    
    v2 |= ( v << 8 );
    v = x2;    
    v2 |= v;
    positiontab[selection] = v2;
}

void highlightClockDate( uint8_t selection, uint16_t color, uint32_t *positiontab ) {
    uint8_t x1, x2, y;
    
    y =  ( positiontab[selection] & 0xff0000 ) >> 16;
    x1 = ( positiontab[selection] & 0xff00 ) >> 8;
    x2 = ( positiontab[selection] & 0xff );   
    highlightEntryLineMid( y , color, x1, x2-x1 );    
}

void writeDateTime( int dt ) {
    uint8_t formatted[2];
  
    formatted[1] = dt % 10;
    formatted[0] = dt / 10;
     
    for ( int i=0;i<2;i++) {
        GFXChar ch = gfxchars[ hexchars[formatted[i]] ];
        writeChars(&ch, 1);    
    }            
}

void rndOptions2(APP_CONTEXT* ctx) {
    CTX_OPTIONS2 *sctx;
    sctx = (CTX_OPTIONS2*) (ctx->ctxbuffer + ctx->ctxptr);         
        
    if ( ctx->rinf == REFRESH || ctx->rinf == ANIMATION ) {
        uint32_t positiontab[8];        
        uint8_t y;
        uint8_t x1, x2;
                           
        if ( ctx->rinf == REFRESH ) {
            clearScreen(COLOR_BLACK);
            locate(0, 0);
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_HEAD_OPTIONS2 ]));
            drawFastHLine(0,16, DISPLAY_WIDTH, COLOR_GRAY);
        }
        
        y = 20;
        setPositiontab(0,  0, 0, 25, positiontab );                
        drawImage( 5, y, &bitmaps[SYSICON_CLOCK] );
        
        //write hour
        locate(25, y);
        x1 = getLocationX();
        writeDateTime( sctx->time.tm_hour );
        x2 = getLocationX();
        setPositiontab(1, 0, x1, x2, positiontab );
        drawImage( x2, y, &bitmaps[SYSICON_COLON] );
        x1 = x2 + 4;
        locate(x1, y);
        
        //write minute        
        writeDateTime( sctx->time.tm_min );
        x2 = getLocationX();
        setPositiontab(2, 0, x1, x2, positiontab );
        drawImage( x2, y, &bitmaps[SYSICON_COLON] );
        x1= x2 + 4;
        locate(x1, y);

        //write seconds        
        writeDateTime( sctx->time.tm_sec );
        x2 = getLocationX();
        setPositiontab(3, 0, x1, x2, positiontab );        
        fillRect(x2,y, DISPLAY_WIDTH-x2,15, COLOR_BLACK);
               
        y += 17;
        setPositiontab(4,  1, 0, 25, positiontab );                
        drawImage( 5, y, &bitmaps[SYSICON_CALENDAR] );
        
        //write day
        locate(25, y);
        x1 = getLocationX();
        writeDateTime( sctx->time.tm_mday );
        x2 = getLocationX();
        setPositiontab(5, 1, x1, x2, positiontab );
        drawImage( x2, y, &bitmaps[SYSICON_DOT] );
        x1 = x2 +4;
        locate(x1, y);
        
        //write month
        writeDateTime( sctx->time.tm_mon );
        x2 = getLocationX();
        setPositiontab(6, 1, x1, x2, positiontab );
        drawImage( x2, y, &bitmaps[SYSICON_DOT] );  
        x1 = x2 +4;
        locate(x1, y);

        //write year        
        writeChars(&gfxchars[ hexchars[2] ], 1);         
        writeChars(&gfxchars[ hexchars[0] ], 1);
        writeDateTime( sctx->time.tm_year );
        x2 = getLocationX();
        setPositiontab(7, 1, x1, x2, positiontab );        
        fillRect(x2,y, DISPLAY_WIDTH-x2,15, COLOR_BLACK);
                              
        if ( sctx->oselected != 0xff ) highlightEntryLine( sctx->oselected < 4 ? 0 : 1 , COLOR_BLACK, DISPLAY_WIDTH );
        highlightClockDate( sctx->selected, device_options.highlight_color1, positiontab );
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
        case ENTRY_DETAIL:
            rndEntryDetail(ctx);
            break;
        case EDIT_ENTRY:
            rndEditEntry(ctx);
            break;
        case MESSAGEBOX:
            rndMessagebox(ctx);
            break;
        case CHOOSEBOX:
            rndChoosebox(ctx);
            break;
        case VIEW_TOKEN:
            rndViewToken(ctx);
            break; 
        case OPTIONS1:
            rndOptions1(ctx);
            break;              
        case OPTIONS2:
            rndOptions2(ctx);
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