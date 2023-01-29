/*
 * File:   display_driver.c
 * Author: greengravity
 *
 * Created on 14. Januar 2023, 16:01
 */

#include "mcc_generated_files/system.h"
#include "mcc_generated_files/spi1_driver.h"
#include "mcc_generated_files/pin_manager.h"
#include "mcc_generated_files/oc1.h"
#include "mcc_ext.h"
#include "main.h"
#include "display_driver.h"
#include <stdio.h>
#include <string.h>

#define OC1_TOPVALUE 0x3F80 

static uint8_t _height, _width, _rotation;
static uint8_t _charx=0, _chary=0;
static uint8_t _brightness = 100;

const uint8_t
  Bcmd[] = {                        // Init commands for 7735B screens
    18,                             // 18 commands in list:
    ST77XX_SWRESET,   ST_CMD_DELAY, //  1: Software reset, no args, w/delay
      50,                           //     50 ms delay
    ST77XX_SLPOUT,    ST_CMD_DELAY, //  2: Out of sleep mode, no args, w/delay
      255,                          //     255 = max (500 ms) delay
    ST77XX_COLMOD,  1+ST_CMD_DELAY, //  3: Set color mode, 1 arg + delay:
      0x05,                         //     16-bit color
      10,                           //     10 ms delay
    ST7735_FRMCTR1, 3+ST_CMD_DELAY, //  4: Frame rate control, 3 args + delay:
      0x00,                         //     fastest refresh
      0x06,                         //     6 lines front porch
      0x03,                         //     3 lines back porch
      10,                           //     10 ms delay
    ST77XX_MADCTL,  1,              //  5: Mem access ctl (directions), 1 arg:
      0x08,                         //     Row/col addr, bottom-top refresh
    ST7735_DISSET5, 2,              //  6: Display settings #5, 2 args:
      0x15,                         //     1 clk cycle nonoverlap, 2 cycle gate
                                    //     rise, 3 cycle osc equalize
      0x02,                         //     Fix on VTL
    ST7735_INVCTR,  1,              //  7: Display inversion control, 1 arg:
      0x0,                          //     Line inversion
    ST7735_PWCTR1,  2+ST_CMD_DELAY, //  8: Power control, 2 args + delay:
      0x02,                         //     GVDD = 4.7V
      0x70,                         //     1.0uA
      10,                           //     10 ms delay
    ST7735_PWCTR2,  1,              //  9: Power control, 1 arg, no delay:
      0x05,                         //     VGH = 14.7V, VGL = -7.35V
    ST7735_PWCTR3,  2,              // 10: Power control, 2 args, no delay:
      0x01,                         //     Opamp current small
      0x02,                         //     Boost frequency
    ST7735_VMCTR1,  2+ST_CMD_DELAY, // 11: Power control, 2 args + delay:
      0x3C,                         //     VCOMH = 4V
      0x38,                         //     VCOML = -1.1V
      10,                           //     10 ms delay
    ST7735_PWCTR6,  2,              // 12: Power control, 2 args, no delay:
      0x11, 0x15,
    ST7735_GMCTRP1,16,              // 13: Gamma Adjustments (pos. polarity), 16 args + delay:
      0x09, 0x16, 0x09, 0x20,       //     (Not entirely necessary, but provides
      0x21, 0x1B, 0x13, 0x19,       //      accurate colors)
      0x17, 0x15, 0x1E, 0x2B,
      0x04, 0x05, 0x02, 0x0E,
    ST7735_GMCTRN1,16+ST_CMD_DELAY, // 14: Gamma Adjustments (neg. polarity), 16 args + delay:
      0x0B, 0x14, 0x08, 0x1E,       //     (Not entirely necessary, but provides
      0x22, 0x1D, 0x18, 0x1E,       //      accurate colors)
      0x1B, 0x1A, 0x24, 0x2B,
      0x06, 0x06, 0x02, 0x0F,
      10,                           //     10 ms delay
    ST77XX_CASET,   4,              // 15: Column addr set, 4 args, no delay:
      0x00, 0x02,                   //     XSTART = 2
      0x00, 0x81,                   //     XEND = 129
    ST77XX_RASET,   4,              // 16: Row addr set, 4 args, no delay:
      0x00, 0x02,                   //     XSTART = 1
      0x00, 0x81,                   //     XEND = 160
    ST77XX_NORON,     ST_CMD_DELAY, // 17: Normal display on, no args, w/delay
      10,                           //     10 ms delay
    ST77XX_DISPON,    ST_CMD_DELAY, // 18: Main screen turn on, no args, delay
      255 },                        //     255 = max (500 ms) delay

  Rcmd1[] = {                       // 7735R init, part 1 (red or green tab)
    15,                             // 15 commands in list:
    ST77XX_SWRESET,   ST_CMD_DELAY, //  1: Software reset, 0 args, w/delay
      150,                          //     150 ms delay
    ST77XX_SLPOUT,    ST_CMD_DELAY, //  2: Out of sleep mode, 0 args, w/delay
      255,                          //     500 ms delay
    ST7735_FRMCTR1, 3,              //  3: Framerate ctrl - normal mode, 3 arg:
      0x01, 0x2C, 0x2D,             //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3,              //  4: Framerate ctrl - idle mode, 3 args:
      0x01, 0x2C, 0x2D,             //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6,              //  5: Framerate - partial mode, 6 args:
      0x01, 0x2C, 0x2D,             //     Dot inversion mode
      0x01, 0x2C, 0x2D,             //     Line inversion mode
    ST7735_INVCTR,  1,              //  6: Display inversion ctrl, 1 arg:
      0x07,                         //     No inversion
    ST7735_PWCTR1,  3,              //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                         //     -4.6V
      0x84,                         //     AUTO mode
    ST7735_PWCTR2,  1,              //  8: Power control, 1 arg, no delay:
      0xC5,                         //     VGH25=2.4C VGSEL=-10 VGH=3 * AVDD
    ST7735_PWCTR3,  2,              //  9: Power control, 2 args, no delay:
      0x0A,                         //     Opamp current small
      0x00,                         //     Boost frequency
    ST7735_PWCTR4,  2,              // 10: Power control, 2 args, no delay:
      0x8A,                         //     BCLK/2,
      0x2A,                         //     opamp current small & medium low
    ST7735_PWCTR5,  2,              // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1,  1,              // 12: Power control, 1 arg, no delay:
      0x0E,
    ST77XX_INVOFF,  0,              // 13: Don't invert display, no args
    ST77XX_MADCTL,  1,              // 14: Mem access ctl (directions), 1 arg:
      0xC8,                         //     row/col addr, bottom-top refresh
    ST77XX_COLMOD,  1,              // 15: set color mode, 1 arg, no delay:
      0x05 },                       //     16-bit color

  Rcmd2green[] = {                  // 7735R init, part 2 (green tab only)
    2,                              //  2 commands in list:
    ST77XX_CASET,   4,              //  1: Column addr set, 4 args, no delay:
      0x00, 0x02,                   //     XSTART = 0
      0x00, 0x7F+0x02,              //     XEND = 127
    ST77XX_RASET,   4,              //  2: Row addr set, 4 args, no delay:
      0x00, 0x01,                   //     XSTART = 0
      0x00, 0x9F+0x01 },            //     XEND = 159

  Rcmd2red[] = {                    // 7735R init, part 2 (red tab only)
    2,                              //  2 commands in list:
    ST77XX_CASET,   4,              //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,                   //     XSTART = 0
      0x00, 0x7F,                   //     XEND = 127
    ST77XX_RASET,   4,              //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,                   //     XSTART = 0
      0x00, 0x9F },                 //     XEND = 159

  Rcmd2green144[] = {               // 7735R init, part 2 (green 1.44 tab)
    2,                              //  2 commands in list:
    ST77XX_CASET,   4,              //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,                   //     XSTART = 0
      0x00, 0x7F,                   //     XEND = 127
    ST77XX_RASET,   4,              //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,                   //     XSTART = 0
      0x00, 0x7F },                 //     XEND = 127

  Rcmd2green160x80[] = {            // 7735R init, part 2 (mini 160x80)
    2,                              //  2 commands in list:
    ST77XX_CASET,   4,              //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,                   //     XSTART = 0
      0x00, 0x4F,                   //     XEND = 79
    ST77XX_RASET,   4,              //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,                   //     XSTART = 0
      0x00, 0x9F },                 //     XEND = 159

  Rcmd3[] = {                       // 7735R init, part 3 (red or green tab)
    4,                              //  4 commands in list:
    ST7735_GMCTRP1, 16      ,       //  1: Gamma Adjustments (pos. polarity), 16 args + delay:
      0x02, 0x1c, 0x07, 0x12,       //     (Not entirely necessary, but provides
      0x37, 0x32, 0x29, 0x2d,       //      accurate colors)
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      ,       //  2: Gamma Adjustments (neg. polarity), 16 args + delay:
      0x03, 0x1d, 0x07, 0x06,       //     (Not entirely necessary, but provides
      0x2E, 0x2C, 0x29, 0x2D,       //      accurate colors)
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST77XX_NORON,     ST_CMD_DELAY, //  3: Normal display on, no args, w/delay
      10,                           //     10 ms delay
    ST77XX_DISPON,    ST_CMD_DELAY, //  4: Main screen turn on, no args w/delay
      100 };                        //     100 ms delay


void dispWriteCommand(uint8_t cmd) {
  ST77XX_PIN_DC_LOW;
  spi1_exchangeByte(cmd);
  ST77XX_PIN_DC_HIGH;
}


void dispSendCommand(uint8_t commandByte, uint8_t *dataBytes,
                                  uint8_t numDataBytes) {
  ST77XX_PIN_CS_LOW;

  dispWriteCommand(commandByte);
  
  for (int i = 0; i < numDataBytes; i++) {
    spi1_exchangeByte(*dataBytes); // Send the data bytes 
    dataBytes++;
  }
  
  ST77XX_PIN_CS_HIGH;
}

void dispSendCommandC(uint8_t commandByte, const uint8_t *dataBytes,
                                  uint8_t numDataBytes) {
  ST77XX_PIN_CS_LOW;

  dispWriteCommand(commandByte);
  
  for (int i = 0; i < numDataBytes; i++) {
    spi1_exchangeByte(*dataBytes); // Send the data bytes 
    dataBytes++;
  }
  
  ST77XX_PIN_CS_HIGH;
}


void dispInit(const uint8_t *addr){

  uint8_t numCommands, cmd, numArgs;
  uint16_t ms;

  numCommands = *addr; // Number of commands to follow
  addr++;
  while (numCommands--) {              // For each command...
    cmd = *addr;
    addr++;// Read command
    numArgs = *addr;
    addr++;// Number of args to follow
    ms = numArgs & ST_CMD_DELAY;       // If hibit set, delay follows args
    numArgs &= ~ST_CMD_DELAY;          // Mask out delay bit
    dispSendCommandC(cmd, addr, numArgs);
    addr += numArgs;

    if (ms) {
      ms = *addr;
      addr++;// Read post-command delay time (ms)
      if (ms == 255)
        ms = 500; // If 255, delay for 500 ms
      __delay_ms(ms);
    }
  }   
}


void dispCommonInit(const uint8_t *cmdList) {

  if (cmdList) {
    dispInit(cmdList);
  }    
    
}

void dispSetRotation(uint8_t m) {
  uint8_t madctl = 0;

  _rotation = m % 4; // can't be higher than 3

  switch (_rotation) {
  case 0:
    madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MY | ST77XX_MADCTL_RGB;
    break;
  case 1:
    madctl = ST77XX_MADCTL_MY | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
    break;
  case 2:
    madctl = ST77XX_MADCTL_RGB;
    break;
  case 3:
    madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
    break;
  }

  dispSendCommand(ST77XX_MADCTL, &madctl, 1);
}

void dispSetBrightness(uint8_t brightness ) {
    if ( brightness > 100 ) brightness = 100;
    _brightness = brightness;
    uint32_t b = OC1_TOPVALUE;
    b = ( b * ((uint32_t)brightness) ) / 100ul;
    OC1_PrimaryValueSet( OC1_TOPVALUE - ( (uint16_t)b ) );
}

uint8_t dispGetBrightness() {    
    return _brightness;
}       



void dispStart( ) {
       
  ST77XX_PIN_CS_HIGH;
  ST77XX_PIN_DC_HIGH;

  ST77XX_PIN_RES_HIGH;    
  __delay_ms(100);
  ST77XX_PIN_RES_LOW;    
  __delay_ms(100);
  ST77XX_PIN_RES_HIGH;    
  __delay_ms(200);
       
  dispCommonInit(Rcmd1);
  _width = ST7735_TFTWIDTH;
  _height = ST7735_TFTHEIGHT;
  dispInit(Rcmd2red);
  dispInit(Rcmd3);
  dispSetRotation( 1 );  
  
  clearScreen(COLOR_BLACK);
  
  OC1_SecondaryValueSet(OC1_TOPVALUE);
  dispSetBrightness(100);
  OC1_Start();
}


/*
void dispStop() {
    //while ( OC1_IsCompareCycleComplete( ) );
    ST77XX_PIN_CS_HIGH;
    ST77XX_PIN_DC_HIGH;
    OC1_Stop();    
    ST77XX_PIN_BACKLIGHT_HIGH;      
}
*/

void dispStartWrite(void) {  
    ST77XX_PIN_CS_LOW;
}

/*!
    @brief  Call after issuing command(s) or data to display. Performs
            chip-deselect (if required) and ends an SPI transaction (if
            using hardware SPI and transactions are supported). Required
            for all display types; not an SPI-specific function.
*/
void dispEndWrite(void) {
    ST77XX_PIN_CS_HIGH;
}

void dispSetAddrWindow(uint16_t x1, uint16_t y1, uint16_t x2,
                                    uint16_t y2) {  
  uint32_t xa = ((uint32_t)x1 << 16) | (x2);
  uint32_t ya = ((uint32_t)y1 << 16) | (y2);

  dispWriteCommand(ST77XX_CASET); // Column addr set  
  SPI1_Transmit32bit(xa);

  dispWriteCommand(ST77XX_RASET); // Row addr set
  SPI1_Transmit32bit(ya);

  dispWriteCommand(ST77XX_RAMWR); // write to RAM
}


void drawPixel(int16_t x, int16_t y, uint16_t color) {
  // Clip first...
  if ((x >= 0) && (x < _width) && (y >= 0) && (y < _height)) {
    // THEN set up transaction (if needed) and draw...
    dispStartWrite();
    dispSetAddrWindow(x, y, x, y);
    SPI1_Transmit16bit(color);
    dispEndWrite();
  }    
}

void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {

    uint16_t x2,y2;
    if ( w > 0 ) {
        x2 = x + w - 1;
    } else if ( w < 0 ) {
        x2 = x;
        x += ( w - 1 );
    } else return;

    if ( h >= 0 ) {
        y2 = y + h - 1;
    } else if ( h < 0 ) {
        y2 = y;
        y += ( h - 1 );        
    } else return;
        
    x = x < 0 ? 0 : ( x >= _width ? ( _width - 1 ) : x );
    y = y < 0 ? 0 : ( y >= _height ? ( _height - 1 ) : y );
    x2 = x2 < 0 ? 0 : ( x2 >= _width ? ( _width - 1 ) : x2 );
    y2 = y2 < 0 ? 0 : ( y2 >= _height ? ( _height - 1 ) : y2 );
           
    int len = ( (int)( ( x2-x ) + 1 ) ) * ( (int)( ( y2-y ) + 1 ) );
    dispStartWrite();
    dispSetAddrWindow(x, y, x2, y2);    
    SPI1_Transmit16bitRepeated(color, len);
    dispEndWrite();
}





void drawFastHLine(int16_t x, int16_t y, int16_t length, uint16_t color) {
    fillRect(x, y, length, 1, color);       
}

void drawFastVLine(int16_t x, int16_t y, int16_t length, uint16_t color) {
    fillRect(x, y, 1, length, color);       
}

void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    drawFastHLine(x,y,w, color);
    drawFastHLine(x,y+h,w, color);    

    drawFastVLine(x,y,h, color);
    drawFastVLine(x+w,y,h, color);        
}

void clearScreen(uint16_t color) {
    fillRect(0,0,_width, _height/2, color);    
    fillRect(0,_height/2,_width, _height/2, color);
}

void drawImage(int16_t x, int16_t y, const GFXimage *image) {
           
    uint16_t outbuffer[MAX_IMAGE_BUFFER];
    int16_t gw = image->width;
    int16_t gh = image->height;
    uint16_t size = gw * gh;
    
    if ( ( ( x + gw ) <= 0 ) || 
        ( ( y + gh ) <= 0 ) ||           
        ( x >= _width ) ||           
        ( y >= _height ) ) return; //check image offscreen
      
    for (uint16_t i=0; i< size; i++) {
        outbuffer[i] = bitmapdata[image->bitmapOffset + i];       
    }
    
    dispStartWrite();
    
    if ( x >= 0 && y >= 0 && ( x + gw ) < _width && ( y + gh ) < _height   ) {
        //Image complete on screen, need no clipping
        dispSetAddrWindow(x, y, x + ( gw - 1 ), y + ( gh - 1 ) );
        SPI1_transmit16bitBuffer(outbuffer, gw * gh );
        
    } else {
        //Image will be clipped before draw
        uint8_t _x;
        uint8_t _y;        
        uint8_t _w = gw;
        uint8_t _h = gh;
        uint8_t _xoff = 0;
        uint8_t _yoff = 0;
        
        if ( x >= 0 ) {
            _x = x;
        } else {
            _x = 0;
            _xoff -= x;
            _w += x;
        }         
        
        if ( ( _x + _w ) > _width ) {
            _w = _width - _x;
        }

        if ( y >= 0 ) {
            _y = y;
        } else {
            _y = 0;
            _yoff -= y;
            _h += y;
        }         
        
        if ( ( _y + _h ) > _height ) {
            _h = _height - _y;
        }

        for ( int iy=0; iy<_h; iy++ ) {
            dispSetAddrWindow(_x, _y + iy, _x + ( _w - 1 ), _y + iy );
            SPI1_transmit16bitBuffer(outbuffer + _xoff + (_yoff+iy) * gw , _w );            
        }
        
    }
    dispEndWrite();
 
}

uint8_t unicodeLookup(uint16_t ct) {
    for ( uint8_t c=0;c<TOTAL_CHAR_COUNT;c++ ) {
        if ( unicodes[c].uccp == ct ) {
            return unicodes[c].cid;
        }
    }
    
    int16_t low = 0;
    int16_t high = TOTAL_CHAR_COUNT - 1;
    while ( true ) {
        uint8_t offs = ( high - low ) / 2;
        if ( unicodes[ offs ].uccp > ct ) {
            high = offs - 1;            
            if ( low > high ) return CHAR_ENCODEERR;
        } else if ( unicodes[ offs ].uccp < ct ) {
            low = offs + 1;
            if ( low > high ) return CHAR_ENCODEERR;
        } else return unicodes[ offs ].cid;
    }           
}

//Setting Writecursor
void locate( uint8_t x, uint8_t y ) {
    _charx = x;
    _chary = y;
}

uint8_t getLocationX() {
    return _charx;    
}

uint8_t getLocationY() {
    return _chary;
}


//Writing Characters with intern codepage
void unwriteChars( const GFXChar *chars, uint16_t len ) {

    for (uint16_t i=0;i<len;i++ ) {
        if ( ( _charx + chars[i].xadv ) > DISPLAY_WIDTH ) {
            _charx = 0;
            _chary += CHAR_HEIGHT;
        }
        fillRect( _charx, _chary, chars[i].xadv, CHAR_HEIGHT, COLOR_BLACK );        
        _charx += chars[i].xadv;
    }    
       
}

void writeChars( const GFXChar *chars, uint16_t len ) {    
    for (uint16_t i=0;i<len;i++ ) {
        if ( ( _charx + chars[i].xadv ) > DISPLAY_WIDTH ) {
            _charx = 0;
            _chary += CHAR_HEIGHT;
        }
        drawImage( _charx, _chary, &bitmaps[ chars[i].id ] );
        _charx += chars[i].xadv;
    }    
}


//Writing text from intern Codepage
void cWriteTextIntern( const uint8_t *text ) {    
    uint16_t len = strlen( (char*)text );
    for ( uint16_t i=0;i<len;i++) {        
        writeChars( &gfxchars[ text[i] ], 1 );
    }
}

void writeTextIntern( uint8_t *text ) {    
    uint16_t len = strlen( (char*)text );
    for ( uint16_t i=0;i<len;i++) {        
        writeChars( &gfxchars[ text[i] ], 1 );
    }
}

//Writing possible Characters from ASCII Codepage
void cWriteText( const char *text ) {
    
    uint16_t len = strlen( text );
    for ( uint16_t i=0;i<len;i++) {
        uint16_t ct = (uint16_t)text[i];      
        writeChars( &gfxchars[ unicodeLookup(ct) ], 1 );
    }    
}

void writeText( char *text ) {    
    uint16_t len = strlen( text );
    for ( uint16_t i=0;i<len;i++) {
        uint16_t ct = (uint16_t)text[i];        
        writeChars( &gfxchars[ unicodeLookup(ct) ], 1 );
    }    
}
