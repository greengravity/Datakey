/*
 * File:   ui.c
 * Author: greengravity
 *
 * Created on 20. Januar 2023, 16:08
 */


#include "xc.h"
#include "logic.h"
#include "display_driver.h"
#include "assets.h"
#include "mcc_generated_files/spi1_driver.h"


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

void rndKeyOverview(APP_CONTEXT* ctx) {
    CTX_KEY_OVERVIEW *sctx;
    sctx = (CTX_KEY_OVERVIEW*) (ctx->ctxbuffer + ctx->ctxptr);

    if (ctx->rinf == REFRESH) {
        clearScreen(COLOR_BLACK);
        locate(0, 0);
        cWriteTextIntern((const uint8_t*) (textdata + texte[ TEXT_TEST ]));
        
        locate( 0,20 );
        char text[20];
        sprintf(text, "%u", (unsigned int)sctx->written );
        writeText( text );           
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
        case KEY_OVERVIEW:
            rndKeyOverview(ctx);
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