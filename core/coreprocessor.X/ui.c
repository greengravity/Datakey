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
#include "mcc_ext.h"

#include "sha/sha.h"

//Tetris icons corresponds to the tetrisstones in logic.h
const uint16_t tetrisIcons[] = {
        SYSICON_TETRIS_I,
        SYSICON_TETRIS_O,
        SYSICON_TETRIS_T,
        SYSICON_TETRIS_L,
        SYSICON_TETRIS_J,
        SYSICON_TETRIS_Z,
        SYSICON_TETRIS_S,
        SYSICON_TETRIS_BG, 
};

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
        } else if ( k->fkt == KEYCOMMAND_LF ) {
            icon = SYSICON_ENTER;
        } else { 
            if ( ioctx->inp == TOKEN ) {
                if ( k->fkt == KEYCOMMAND_ABORT ) {
                    icon = SYSICON_ABORT;
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
            }
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
            if ( ioctx->inp == TOKEN ) {
                cWriteTextIntern((const uint8_t*) (textdata + texte[ token_configs[ioctx->type].textid ]));
            } else if ( ioctx->inp == MASTERKEY ) {
                cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_ENTER_NEW_KEY ]));
            }
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
    
    if (ctx->rinf == IO_UPDATE) {                
        rndIO( &sctx->io );                                   
    }    
}    

void writeNumber(uint8_t* text, long nr) {
    uint8_t i=0;
    
    if ( nr == 0 ) {
        text[0] = hexchars[ 0 ];
        text[1] = 0;
        return;
    }
    
    while ( nr > 0 ) {         
        text[i] = hexchars[ nr % 10 ];
        i++;
        nr = nr / 10;
    }
    for ( int j=0;j<i/2;j++) {
        uint8_t a = text[j];
        text[j] = text[i-1-j];
        text[i-1-j] = a;
    }
    text[i] = 0x00;
}

void rndPinInput(APP_CONTEXT* ctx) {
    CTX_PIN_INPUT *sctx;
    sctx = (CTX_PIN_INPUT*) (ctx->ctxbuffer + ctx->ctxptr);
    
    uint8_t x;
    uint8_t text[6];
    
    if ( ctx->rinf == REFRESH || ctx->rinf == ANIMATION ) {
        if ( ctx->rinf == REFRESH ) {
            clearScreen(COLOR_BLACK);
        }        
        //Drawfield
        for ( int x=0;x<8;x++) {
            for ( int y=0;y<8;y++) {
                uint16_t icon = SYSICON_PIN_GROUND;
                if ( sctx->px == x && sctx->py == y ) {                    
                    icon = SYSICON_PIN_MARKER;
                } else {                                                             
                    switch ( sctx->field[x + y * 8] ) {
                        case PFLD_OWL:
                            icon = SYSICON_PIN_OBS1;
                            break;
                        case PFLD_LITTLEDRAGON:
                            icon = SYSICON_PIN_OBS2;
                            break;
                        case PFLD_TREE:
                            icon = SYSICON_PIN_OBS3;
                            break;
                        case PFLD_STONE:
                            icon = SYSICON_PIN_OBS4;
                            break;
                        case PFLD_SHIELD:
                            icon = SYSICON_PIN_OBS5;
                            break;
                        case PFLD_DRAGON:
                            icon = SYSICON_PIN_OBS6;
                            break; 
                        case PFLD_CRACKSTONE:
                            icon = SYSICON_PIN_OBS7;
                            break; 
                        case PFLD_SWORD:
                            icon = SYSICON_PIN_OBS8;
                            break; 
                        case PFLD_GRAIL:
                            icon = SYSICON_PIN_OBS9;
                            break;                 
                    }                                       
                }
                
                drawImage( x*15, y*15 , &bitmaps[icon] );                
            }
        } 
        drawFastVLine(120, 0, DISPLAY_HEIGHT, COLOR_WHITE);
        
        //Drawstats        
        if ( sctx->generatenew ) {
            locate( 124,20 );   
            setWritebounds( 124, DISPLAY_WIDTH );
            if ( sctx->verify ) {
                cWriteTextInternLinebreak((const uint8_t*) (textdata + texte[ TEXT_PIN_INF_GE_VERIFY_PIN ]));
            } else {
                cWriteTextInternLinebreak((const uint8_t*) (textdata + texte[ TEXT_PIN_INF_GE_ENTER_PIN ]));
            }
                        
            locate( 124,80 );
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_PIN_INF_STEPS ]));
            locate( 124,95 );            
            writeNumber(text, device_options.pin_len - sctx->ppos );            
            writeTextIntern( text );
            x = getLocationX();
            fillRect(x, 95, DISPLAY_WIDTH - x, 15, COLOR_BLACK);            
                        
            if ( sctx->error ) {
                fillRect( 10, 28, 150, 64, COLOR_BLACK );
                drawRect( 10, 28, 150, 64, COLOR_WHITE );
                locate( 14, 30 );
                setWritebounds( 14, 146 );
                cWriteTextInternLinebreak((const uint8_t*) (textdata + texte[ TEXT_PIN_INF_GE_PIN_ERROR ]));                
            }            
            
            setWritebounds( 0, DISPLAY_WIDTH );

        } else {          
            locate( 124,20 );            
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_PIN_INF_LEVEL ]));
            locate( 124,35 );
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_PIN_INF_LEVELNR ]));            
            
            locate( 124,55 );
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_PIN_INF_STEPS ]));
            locate( 124,70 );            
            writeNumber(text, device_options.pin_len - sctx->ppos );            
            writeTextIntern( text );                        
            if ( sctx->extrasteps > 0 ) {
                cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_PIN_INF_SPLIT_STEPS ]));
                writeNumber(text, sctx->extrasteps );            
                writeTextIntern( text );                                                  
            }            
            x = getLocationX();              
            fillRect(x, 70, DISPLAY_WIDTH - x, 15, COLOR_BLACK);
                                    
            locate( 124,90 );
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_PIN_INF_LIVES ]));
            locate( 124,105 );              
            writeNumber(text, device_options.pin_tries - sctx->pinerr );            
            writeTextIntern( text );
            

            x = getLocationX();              
            fillRect(x, 105, DISPLAY_WIDTH - x, 15, COLOR_BLACK);                                    
            
            
            if ( sctx->error ) {
                fillRect( 10, 50, 100, 20, COLOR_BLACK );
                drawRect( 10, 50, 100, 20, COLOR_WHITE );
                locate( 14, 52 );
                cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_PIN_INF_NO_STEPS ]));                
            }
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
                drawup = false;
                drawdown = false;
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

int getTextWidth( const uint8_t *text ) {
    int len = strlen( (char*)text );
    int tlen=0;
    
    for (int i=0;i<len;i++) {
        tlen += gfxchars[ text[i] ].xadv;
    }        
    return tlen;
}

void rndEntryDetail( APP_CONTEXT* ctx ) {
    CTX_ENTRY_DETAIL *sctx;
    sctx = (CTX_ENTRY_DETAIL*) (ctx->ctxbuffer + ctx->ctxptr);
    
    int maxlen=0;
    
    if (ctx->rinf == REFRESH) {
        uint8_t y;
        clearScreen(COLOR_BLACK);
        locate(0, 0);
        cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_HEAD_ENTRY_DETAIL ]));
        drawFastHLine(0,16, DISPLAY_WIDTH, COLOR_GRAY);
        
        y = 20;
        for (int i=0;i<sctx->tokencount;i++) {
            int tw = getTextWidth( (const uint8_t*) (textdata + texte[ token_configs[ sctx->tokeninfo[i].type ].tkname ]) );
            if ( tw > maxlen ) maxlen = tw;
        }
        
        for (int i=0;i<sctx->tokencount;i++) {
              
            int tw = getTextWidth( (const uint8_t*) (textdata + texte[ token_configs[ sctx->tokeninfo[i].type ].tkname ]) );
            locate(maxlen - tw, y);
            
            cWriteTextIntern((const uint8_t*) (textdata + texte[ token_configs[ sctx->tokeninfo[i].type ].tkname ]));            
            cWriteTextInternNLB(sctx->tokeninfo[i].text );    
            y += 17;
        }
        
        highlightEntryDetail(sctx->selected, device_options.highlight_color1);
    } else if (ctx->rinf == ANIMATION) {  
        highlightEntryDetail(sctx->oselected, COLOR_BLACK);
        highlightEntryDetail(sctx->selected, device_options.highlight_color1);
    }
}

void rndUSBPush(APP_CONTEXT* ctx) {
    if ( ctx->rinf == REFRESH ) {
        fillRect( 10,20, DISPLAY_WIDTH-20, DISPLAY_HEIGHT-40, COLOR_BLACK );
        drawRect( 10,20, DISPLAY_WIDTH-20, DISPLAY_HEIGHT-40, COLOR_WHITE );

        setWritebounds( 14, DISPLAY_WIDTH-14 );
        locate( 14, 24 );
        cWriteTextInternLinebreak( (const uint8_t*) (textdata + texte[ TEXT_WRITE_USB_OUT ]) );   
        setWritebounds( 0, DISPLAY_WIDTH );                
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

void rngMessageBoxOption( uint16_t icon, uint8_t pos, uint16_t selcolor ) {                
    drawRect( DISPLAY_WIDTH - 35 - pos * 22, DISPLAY_HEIGHT-47, 21, 17, selcolor );
    drawImage(DISPLAY_WIDTH - 31 - pos * 22, DISPLAY_HEIGHT-45, &bitmaps[icon] );
}

void rndMessagebox(APP_CONTEXT* ctx) {
    CTX_MSG_BOX *sctx;
    sctx = (CTX_MSG_BOX*) (ctx->ctxbuffer + ctx->ctxptr);    
    
    if ( ctx->rinf == REFRESH || ctx->rinf == ANIMATION ) {                
        if ( ctx->rinf == REFRESH ) {
            fillRect( 10,20, DISPLAY_WIDTH-20, DISPLAY_HEIGHT-40, COLOR_BLACK );
            drawRect( 10,20, DISPLAY_WIDTH-20, DISPLAY_HEIGHT-40, COLOR_WHITE );
            
            setWritebounds( 14, DISPLAY_WIDTH-14 );
            locate( 14, 24 );
            cWriteTextInternLinebreak( sctx->msgtxt );   
            setWritebounds( 0, DISPLAY_WIDTH );
        }

        for ( uint8_t i=0;i<sctx->choicecount;i++ ) {
            rngMessageBoxOption( sctx->mchoiceicon[i], sctx->choicecount - (i+1), i == sctx->sel ? device_options.highlight_color1 : COLOR_BLACK );
        }        
    }
}

void highlightChooseBoxEntry(uint8_t yoff, uint8_t selection, uint16_t color ) {
    fillRect( 14, yoff + 2 + selection * 17, DISPLAY_WIDTH - 28, 1, color );    
    fillRect( 14, yoff + 2 + selection * 17 + 17, DISPLAY_WIDTH - 28,1, color );    
}

void rndChoosebox(APP_CONTEXT* ctx) {
    
    CTX_CHOOSE_BOX *sctx;
    sctx = (CTX_CHOOSE_BOX*) (ctx->ctxbuffer + ctx->ctxptr);    
    
    if ( ctx->rinf == REFRESH || ctx->rinf == ANIMATION ) {  
        uint8_t height = DISPLAY_HEIGHT-40; //sctx->options * 17 + 6 + 2 * sctx->padding;
        uint8_t yoff = ( DISPLAY_HEIGHT - height ) / 2;
        uint8_t xoff = 10;
        
        if ( ctx->rinf == REFRESH ) {
            fillRect( xoff,yoff, DISPLAY_WIDTH-20, height, COLOR_BLACK );
            drawRect( xoff,yoff, DISPLAY_WIDTH-20, height,  COLOR_WHITE );
        }
        
        setWritebounds( 14, DISPLAY_WIDTH-14 );
        for (uint8_t i =0; i<sctx->options; i++) {
            locate( 14, yoff + 3 + i * 17  );
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

void rndOptions(APP_CONTEXT* ctx) {
    CTX_OPTIONS *sctx;
    sctx = (CTX_OPTIONS*) (ctx->ctxbuffer + ctx->ctxptr);         
        
    if ( ctx->rinf == REFRESH || ctx->rinf == ANIMATION ) {
        uint32_t positiontab[12];        
        uint8_t y;
        uint8_t x1, x2;
                           
        if ( ctx->rinf == REFRESH ) {
            clearScreen(COLOR_BLACK);
            locate(0, 0);
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_HEAD_OPTIONS ]));
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
           
        
        y += 17;
        setPositiontab(8,  2, 0, DISPLAY_WIDTH, positiontab );   
        drawImage( 5, y, &bitmaps[SYSICON_USB] );
        locate(25, y);
        if ( device_options.umode == USB_MODE_OFF ) {
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_OPTV_USB_OFF ]));
        } else if ( device_options.umode == USB_MODE_KEYBOARD ) {
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_OPTV_USB_KEYBOARD ]));            
        }
        x1 = getLocationX();
        fillRect(x1, y, DISPLAY_WIDTH-x1, 15, COLOR_BLACK );        
        
        y += 17;
        setPositiontab(9,  3, 0, DISPLAY_WIDTH, positiontab );
        drawImage( 5, y, &bitmaps[SYSICON_BRIGHTNESS] );
//        fillRect(23, y+1, 2, 13, COLOR_BLACK ); 
//        fillRect(25+64, y+1, 2, 13, COLOR_BLACK );
        int bloc = device_options.brightness;
        bloc = ( bloc * 64 ) / 100;
        
        for (int i=0;i<64;i++) {
            uint16_t gcb;
            uint16_t gradient_color;
                        
            gcb = i*4;
            gradient_color = ( ( gcb & 0xf8 ) << 8 ) | ( ( gcb & 0xfc ) << 3 ) | ( ( gcb & 0xf8 ) >> 3 );
                        
            if ( i == bloc - 3 || i == bloc - 1 ) {
                 fillRect(25+i, y+1, 1, 13, COLOR_BLACK );
            } else if ( i == bloc-2 ) {
                fillRect(25+i, y+1, 1, 13, device_options.highlight_color1 );
            } else fillRect(25+i, y+1, 1, 13, gradient_color );            
        }
      
        y += 17;
        setPositiontab(10,  4, 0, DISPLAY_WIDTH, positiontab ); 
        drawImage( 5, y, &bitmaps[SYSICON_COLOR] );
        fillRect( 25, y+3, 64, 9, device_options.highlight_color1 );

        
        y += 17;
        setPositiontab(11,  5, 0, DISPLAY_WIDTH, positiontab ); 
        drawImage( 5, y, &bitmaps[SYSICON_SPEED] );
        locate(25, y);
        if ( device_options.keymode == QUICKKEY_OFF ) {
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_OPTV_QKEYS_OFF ]));
        } else if ( device_options.keymode == QUICKKEY_ON ) {
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_OPTV_QKEYS_ON ]));            
        }
        x1 = getLocationX();
        fillRect(x1, y, DISPLAY_WIDTH-x1, 15, COLOR_BLACK );  
        
        if ( sctx->oselected != 0xff && sctx->oselected != sctx->selected ) {
            y =  ( positiontab[sctx->oselected] & 0xff0000 ) >> 16;
            highlightEntryLine( y, COLOR_BLACK, DISPLAY_WIDTH );
        }                        
        highlightClockDate( sctx->selected, device_options.highlight_color1, positiontab );
    }    
}

void rndPinOptions(APP_CONTEXT* ctx) {
    CTX_PINOPTIONS *sctx;
    sctx = (CTX_PINOPTIONS*) (ctx->ctxbuffer + ctx->ctxptr);
    
    uint8_t text[10];
    
    if ( ctx->rinf == REFRESH || ctx->rinf == ANIMATION ) {
        uint8_t y, x1;
                           
        if ( ctx->rinf == REFRESH ) {
            clearScreen(COLOR_BLACK);
            locate(0, 0);
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_HEAD_PINOPTIONS ]));
            drawFastHLine(0,16, DISPLAY_WIDTH, COLOR_GRAY);
        }           
       
        y = 20;
        locate(5, y);
        cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_OPT_CHANGE_PIN ]));
        
        
        y += 17;
        int maxlen=0, tw;
        tw = getTextWidth( (const uint8_t*) (textdata + texte[ TEXT_OPT_PIN_TRIES ]) );
        if ( tw > maxlen ) maxlen = tw;
        tw = getTextWidth( (const uint8_t*) (textdata + texte[ TEXT_OPT_PIN_LEN ]) );
        if ( tw > maxlen ) maxlen = tw;

        tw = getTextWidth( (const uint8_t*) (textdata + texte[ TEXT_OPT_PIN_TRIES ]) );
        locate(5 + maxlen - tw , y);
        cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_OPT_PIN_TRIES ]));
        writeNumber(text, device_options.pin_tries );            
        writeTextIntern( text ); 
        x1 = getLocationX();
        fillRect(x1, y, DISPLAY_WIDTH-x1, 15, COLOR_BLACK );   
                        
        y += 17;
        tw = getTextWidth( (const uint8_t*) (textdata + texte[ TEXT_OPT_PIN_LEN ]) );
        locate(5 + maxlen - tw , y);
        cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_OPT_PIN_LEN ]));
        writeNumber(text, sctx->pin_len );            
        writeTextIntern( text ); 
        x1 = getLocationX();
        fillRect(x1, y, DISPLAY_WIDTH-x1, 15, COLOR_BLACK );   
        
        
        y += 17;
        locate(5, y);
        setWritebounds( 5, DISPLAY_WIDTH );
        if ( sctx->pin_len != device_options.pin_len ) {
            cWriteTextInternLinebreak((const uint8_t*) (textdata + texte[ TEXT_OPTINF_PIN_LEN_CHANGED ]));
        } else {
            fillRect(0, y, DISPLAY_WIDTH, 15+17, COLOR_BLACK );
        }
        setWritebounds( 0, DISPLAY_WIDTH );        
        
        if ( sctx->oselected != sctx->selected ) {            
            highlightEntryLine( sctx->oselected, COLOR_BLACK, DISPLAY_WIDTH );
        }                                
        highlightEntryLine( sctx->selected,  device_options.highlight_color1, DISPLAY_WIDTH );    
    }
}

void rndDeviceStatus( APP_CONTEXT* ctx ) {
    if ( ctx->dcstate == DEVCST_BATTERY ) {
        drawRect(DISPLAY_WIDTH-23, 3, 20, 8, COLOR_WHITE );
        fillRect(DISPLAY_WIDTH-25, 5, 2, 4, COLOR_WHITE );
        
        fillRect(DISPLAY_WIDTH-25, 3, 2, 2, COLOR_BLACK );
        fillRect(DISPLAY_WIDTH-25, 9, 2, 4, COLOR_BLACK );
        
        fillRect(DISPLAY_WIDTH-25, 0, 25, 3, COLOR_BLACK );
        fillRect(DISPLAY_WIDTH-25, 12, 25, 4, COLOR_BLACK );
        
        fillRect(DISPLAY_WIDTH-2, 3, 2, 9, COLOR_BLACK );
        
        drawRect(DISPLAY_WIDTH-22, 4, 18, 6, COLOR_BLACK );
                
        int chst = ctx->power;
        uint16_t color;
        color = COLOR_GREEN;            
        if ( chst < 20 ) {
            color = COLOR_RED;            
        } else if ( chst < 50 ) {
            color = COLOR_ORANGE;
        }
        
        chst = ( chst * 17 ) / 100;                     
        fillRect(( DISPLAY_WIDTH-21 ) + ( 17 - chst ) , 5, chst, 5, color );                       
        fillRect(DISPLAY_WIDTH-21, 5, 17- chst, 5, COLOR_BLACK );
        
    } else if ( ctx->dcstate == DEVCST_CHARGER || ctx->dcstate == DEVCST_USB  ) {
        if ( ctx->dcstate == DEVCST_CHARGER ) {
            drawImage( DISPLAY_WIDTH-25 , 0, &bitmaps[SYSICON_CHARGE] );
        } else if ( ctx->dcstate == DEVCST_USB ) {
            drawImage( DISPLAY_WIDTH-25 , 0, &bitmaps[SYSICON_USB] );
        }
        
        if ( isBUSChargeEnabled() ) {
            if ( !CHRG_GetValue() ) {
                drawImage( DISPLAY_WIDTH-10 , 0, &bitmaps[SYSICON_CHARGING] );
            } else {
                drawImage( DISPLAY_WIDTH-10 , 0, &bitmaps[SYSICON_CHARGED] );
            }            
        } else {
            drawImage( DISPLAY_WIDTH-10 , 0, &bitmaps[SYSICON_NOCHARGE] );
        }
        
        //fillRect(DISPLAY_WIDTH-5, 0, 5, 15, COLOR_BLACK );
        //fillRect(DISPLAY_WIDTH-25, 0, 5, 15, COLOR_BLACK );
    }
       
/*    char text[20];
    sprintf(text, "%d", ctx->analogtest );
    locate (10,100);
    writeText( text ); */
    
}

void rndTetris(APP_CONTEXT* ctx) {
    CTX_TETRIS *sctx;
    sctx = (CTX_TETRIS*) (ctx->ctxbuffer + ctx->ctxptr); 
    
    uint8_t text[20];
    
    uint8_t yoff=DISPLAY_HEIGHT;        
    if ( ctx->rinf == REFRESH || ctx->rinf == ANIMATION || ctx->rinf == ANIMATION2 ) {
        if ( ctx->rinf == REFRESH ) {
            clearScreen(COLOR_BLACK);
            
            for (int i=0;i<20;i++) {
                drawImage( i*8, yoff - 8, &bitmaps[SYSICON_TETRIS_WALL] );
                drawImage( i*8, yoff - 12*8, &bitmaps[SYSICON_TETRIS_WALL] );
            }
        }
                  
        uint8_t shape[16];
        uint8_t size;
                            
        int fstartx;
        int fendx;
        int fstarty;
        int fendy;
        uint8_t tw;
        uint8_t x;
                      
        if ( ctx->rinf == ANIMATION || ctx->rinf == REFRESH ) {
            locate( 0,2 );
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_TETRIS_SCORE ]));        
            locate( 0,15 );
            writeNumber(text, sctx->score );
            writeTextIntern( text );
            x = getLocationX();
            fillRect(x, 15, 55-x, 15, COLOR_BLACK );
                    
            locate( 55,2 );
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_TETRIS_LINES ]));        
            locate( 55,15 );
            writeNumber(text, sctx->lines );
            writeTextIntern( text );
            x = getLocationX();
            fillRect(x, 15, 100-x, 15, COLOR_BLACK );            
            
            locate( 100,2 );
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_TETRIS_LEVEL ]));        
            locate( 100,15 );         
            writeNumber( text, sctx->lvl );
            writeTextIntern( text );
            x = getLocationX();
            fillRect(x, 15, ( DISPLAY_WIDTH - (8*4) ) - x, 15, COLOR_BLACK );
           
            size = tetris_piecesize[sctx->next];
                                   
            //position the current shape in the field    
            tet_getRotatedShape(shape, sctx->next, 0 );
            
            int endrow = 2;
            if ( sctx->next == TETS_I) endrow = 1;
            for (int x= 0;x<4;x++) {
                for (int y=0;y<2;y++) {                    
                    if ( shape[ x + (y) * 4] ) {                    
                        drawImage( DISPLAY_WIDTH - (4*8) + x*8, DISPLAY_HEIGHT - (14*8) + (8*y) , &bitmaps[tetrisIcons[sctx->next]] );
                    } else {
                        fillRect( DISPLAY_WIDTH - (4*8) + x*8, DISPLAY_HEIGHT - (14*8) + (8*y) , 8,8, COLOR_BLACK );                        
                    }
                }
            }
            
        }
               
        size = tetris_piecesize[sctx->curr];

        if ( ctx->rinf == ANIMATION2 ) {
            fstarty = sctx->curry - 1 < 0 ? 0 : sctx->curry - 1;
            fendy = sctx->curry + size > 20 ? 20 : sctx->curry + size;
            fstartx = sctx->currx - 3 < 0 ? 0 : sctx->currx - 3;
            fendx = sctx->currx + size + 3 > 10 ? 10 : sctx->currx + size + 3;   
            
        } else {
            fstartx = 0; 
            fendx = 10;
            fstarty = 0;
            fendy = 20;
        }
        
        //position the current shape in the field    
        tet_getRotatedShape(shape, sctx->curr, sctx->currrot );          
        
        
        for ( int y=fstarty;y<fendy;y++) {
            for ( int x=fstartx;x<fendx;x++) {
                TETRIS_STONES ts = sctx->field[ x + y * 10 ];
                                
                int sx = x-sctx->currx;
                int sy = y-sctx->curry;
                if ( sx >= 0 && sx < size && sy >= 0 && sy < size && shape[ sx + sy * 4] != 0 ) {
                    ts = sctx->curr;
                }
                   
                drawImage( y*8, yoff - ( 16 + 8 * x ), &bitmaps[tetrisIcons[ts]] );                                             
            }
        }  

        if ( sctx->gameover ) {
            fillRect(30, 70, 100, 40, COLOR_BLACK );
            drawRect(30, 70, 100, 40, COLOR_WHITE );
            
            tw = getTextWidth( (const uint8_t*) (textdata + texte[ TEXT_TETRIS_GAME_OVER1 ]) );
            locate( (DISPLAY_WIDTH - tw) / 2, 73 );
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_TETRIS_GAME_OVER1 ]));

            tw = getTextWidth( (const uint8_t*) (textdata + texte[ TEXT_TETRIS_GAME_OVER2 ]) );
            locate( (DISPLAY_WIDTH - tw) / 2, 90 );
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_TETRIS_GAME_OVER2 ]));
            
            
        } else if ( sctx->startpause ) {
            
            fillRect(30, 70, 100, 40, COLOR_BLACK );
            drawRect(30, 70, 100, 40, COLOR_WHITE );
            
            tw = getTextWidth( (const uint8_t*) (textdata + texte[ TEXT_TETRIS_GAME_START1 ]) );
            locate( (DISPLAY_WIDTH - tw) / 2, 73 );
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_TETRIS_GAME_START1 ]));

            tw = getTextWidth( (const uint8_t*) (textdata + texte[ TEXT_TETRIS_GAME_START2 ]) );
            locate( (DISPLAY_WIDTH - tw) / 2, 90 );
            cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_TETRIS_GAME_START2 ]));
            
        }

        
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
        case USB_PUSH: {
            rndUSBPush(ctx);
            break;            
        }
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
        case OPTIONS:
            rndOptions(ctx);
            break;
        case PINOPTIONS:
            rndPinOptions(ctx);
            break;
        case GAME_TETRIS:
            rndTetris(ctx);
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
                    cWriteTextInternLinebreak((const uint8_t*) (textdata + texte[ TEXT_ERROR_CONTEXT ]));
                } else if ( ctx->ctxtype == ERROR_SD_CD ) {
                    cWriteTextInternLinebreak((const uint8_t*) (textdata + texte[ TEXT_ERROR_SD_CD ]));                    
                } else if ( ctx->ctxtype == ERROR_SD_FAILURE ) {
                    cWriteTextInternLinebreak((const uint8_t*) (textdata + texte[ TEXT_ERROR_SD_FAILURE ]));
                }                        
            }
            break;
    }

    ctx->rinf = UNCHANGED;

    rndDeviceStatus( ctx );
    
    spi1_close();
}


/*
 * Wrting Hexdata
    for (int16_t i = 0; i < 32; i++) {
        GFXChar ch = gfxchars[ hexchars[ ( hash[ i ] >> 4 ) ] ];                
        writeChars(&ch, 1);

        GFXChar ch2 = gfxchars[ hexchars[ ( hash[ i ] & 0x0f ) ] ];                
        writeChars(&ch2, 1);

        GFXChar ch3 = gfxchars[ CHAR_SPACE ];
        writeChars(&ch3, 1);
    }            
*/    
