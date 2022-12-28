/*#define FCY     8000000UL
#include <libpic30.h>
#include "display.h"
#include "mcc_generated_files/pin_manager.h"
#include "mcc_generated_files/spi1_driver.h" */
#include "display_driver.h"


static const uint8_t PROGMEM
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
  SPI1_Exchange8bit(cmd);
  ST77XX_PIN_DC_HIGH;
}


void Adafruit_SPITFT::sendCommand(uint8_t commandByte, uint8_t *dataBytes,
                                  uint8_t numDataBytes) {
  ST77XX_PIN_CS_LOW;

  ST77XX_PIN_DC_LOW;         // Command mode  
  SPI1_Exchange8bit(commandByte); // Send the command byte
  ST77XX_PIN_DC_HIGH;
  
  for (int i = 0; i < numDataBytes; i++) {
    SPI1_Exchange8bit(*dataBytes); // Send the data bytes 
    dataBytes++;
  }
  
  ST77XX_PIN_CS_HIGH;
}

void Adafruit_SPITFT::sendCommand(uint8_t commandByte, const uint8_t *dataBytes,
                                  uint8_t numDataBytes) {
  ST77XX_PIN_CS_LOW;

  ST77XX_PIN_DC_LOW;         // Command mode  
  SPI1_Exchange8bit(commandByte); // Send the command byte
  ST77XX_PIN_DC_HIGH;
  
  for (int i = 0; i < numDataBytes; i++) {
    SPI1_Exchange8bit(pgm_read_byte(dataBytes++)); // Send the data bytes 
    dataBytes++;
  }
  
  ST77XX_PIN_CS_HIGH;
}


void dispBegin(uint32_t freq = 0);

void dispSetColRowStart(int8_t col, int8_t row);




void dispInit(const uint8_t *addr){

  uint8_t numCommands, cmd, numArgs;
  uint16_t ms;

  numCommands = pgm_read_byte(addr++); // Number of commands to follow
  while (numCommands--) {              // For each command...
    cmd = pgm_read_byte(addr++);       // Read command
    numArgs = pgm_read_byte(addr++);   // Number of args to follow
    ms = numArgs & ST_CMD_DELAY;       // If hibit set, delay follows args
    numArgs &= ~ST_CMD_DELAY;          // Mask out delay bit
    sendCommand(cmd, addr, numArgs);
    addr += numArgs;

    if (ms) {
      ms = pgm_read_byte(addr++); // Read post-command delay time (ms)
      if (ms == 255)
        ms = 500; // If 255, delay for 500 ms
      delay(ms);
    }
  }
    
}


void dispCommonInit(const uint8_t *cmdList) {

  if (cmdList) {
    displayInit(cmdList);
  }    
    
}


void setupDisplay( uint8_t options  ) {
  dispCommonInit(Rcmd1);
  if (options == INITR_GREENTAB) {
    displayInit(Rcmd2green);
    _colstart = 2;
    _rowstart = 1;
  } else if ((options == INITR_144GREENTAB) || (options == INITR_HALLOWING)) {
    _height = ST7735_TFTHEIGHT_128;
    _width = ST7735_TFTWIDTH_128;
    displayInit(Rcmd2green144);
    _colstart = 2;
    _rowstart = 3; // For default rotation 0
  } else if (options == INITR_MINI160x80) {
    _height = ST7735_TFTWIDTH_80;
    _width = ST7735_TFTHEIGHT_160;
    displayInit(Rcmd2green160x80);
    _colstart = 24;
    _rowstart = 0;
  } else {
    // colstart, rowstart left at default '0' values
    displayInit(Rcmd2red);
  }
  displayInit(Rcmd3);

  // Black tab, change MADCTL color filter
  if ((options == INITR_BLACKTAB) || (options == INITR_MINI160x80)) {
    uint8_t data = 0xC0;
    sendCommand(ST77XX_MADCTL, &data, 1);
  }

  if (options == INITR_HALLOWING) {
    // Hallowing is simply a 1.44" green tab upside-down:
    tabcolor = INITR_144GREENTAB;
    setRotation(2);
  } else {
    tabcolor = options;
    setRotation(0);
  }    
    
}



/*

void setupDisplay(SSD1306_DISPLAY* display, uint8_t pinid ) {
    display->pinid = pinid;
    display->graphicBuffer = NULL;
    display->xoff = 0;
    display->bufferwidth = DISPLAY_WIDTH;
}

void setGraphicbuffer(SSD1306_DISPLAY* display, uint8_t *graphicBuffer, uint16_t bufferwidth, uint16_t xoff ) {
    display->graphicBuffer = graphicBuffer;
    display->bufferwidth = bufferwidth;
    display->xoff = xoff;
}

void ssd1306_command(SSD1306_DISPLAY* display, uint8_t command ) {
    
    PIN_DISPLAY_DC_LOW
    PIN_DISPLAY_CS_LOW
        
    //spi1_exchangeByte( command );  
    SPI1_Exchange8bit( command );
        
    PIN_DISPLAY_CS_HIGH
}

void deselectDisplay(SSD1306_DISPLAY* display) {    
    PIN_DISPLAY_CS_HIGH    
    PIN_DISPLAY_DC_LOW
}



void startDisplay(SSD1306_DISPLAY* display) {
    
    //spi1_master_open( DISPLAY );
    //spi_master_open( DISPLAY );    
    PIN_DISPLAY_CS_HIGH;    
    PIN_DISPLAY_DC_LOW;
    PIN_DISPLAY_CS_LOW;
            
    PIN_DISPLAY_RES_HIGH;
    __delay_ms(1);
    PIN_DISPLAY_RES_LOW;
    __delay_ms(5);
    PIN_DISPLAY_RES_HIGH;    
    __delay_ms(100);
                
    PIN_DISPLAY_CS_HIGH;
        
    ssd1306_command(display, SSD1306_DISPLAYOFF);                    // 0xAE

    ssd1306_command(display, SSD1306_SETMULTIPLEX);                  // 0xA8
    ssd1306_command(display, SSD1306_LCDHEIGHT - 1);

    ssd1306_command(display, SSD1306_SETDISPLAYOFFSET);              // 0xD3
    ssd1306_command(display, 0x0);                                   // no offset
    
    ssd1306_command(display, SSD1306_SETSTARTLINE);                  // line #0

    ssd1306_command(display, SSD1306_SEGREMAP );
    
    ssd1306_command(display, SSD1306_COMSCANINC);    
    
    ssd1306_command(display, SSD1306_SETCOMPINS);                    // 0xDA
    ssd1306_command(display, 0x12);
    
    ssd1306_command(display, SSD1306_SETCONTRAST);                   // 0x81
    ssd1306_command(display, 0xCF);

    ssd1306_command(display, SSD1306_DISPLAYALLON_RESUME);           // 0xA4
    ssd1306_command(display, SSD1306_NORMALDISPLAY);                 // 0xA6
    
    ssd1306_command(display, SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
    ssd1306_command(display, 0x80);                                  // the suggested ratio 0x80 
    
    ssd1306_command(display, SSD1306_CHARGEPUMP);                    // 0x8D
    ssd1306_command(display, 0x14);   
    
    ssd1306_command(display, SSD1306_SETPRECHARGE);                  // 0xd9
    ssd1306_command(display, 0xF1);
    
    ssd1306_command(display, SSD1306_MEMORYMODE);                    // 0x20
    ssd1306_command(display, 0x00);                                  // 0x0 act like ks0108  
    
    ssd1306_command(display, SSD1306_DISPLAYON);//--turn on oled panel
     
    //spi1_close();
}



void writeDisplay(SSD1306_DISPLAY* display) {
    //spi1_master_open( DISPLAY );
    //spi_master_open( DISPLAY );
    
    ssd1306_command(display, SSD1306_COLUMNADDR);
    ssd1306_command(display, 0);   // Column start address (0 = reset)
    ssd1306_command(display, SSD1306_LCDWIDTH-1); // Column end address (127 = reset)

    ssd1306_command(display, SSD1306_PAGEADDR);
    ssd1306_command(display, 0); // Page start address (0 = reset)        
    ssd1306_command(display, 7); // Page end address            

    PIN_DISPLAY_CS_HIGH;
    PIN_DISPLAY_DC_HIGH;
    PIN_DISPLAY_CS_LOW;

    int i, page;
    //uint8_t inbuffer[SSD1306_LCDWIDTH];
    for (i=0;i<SSD1306_LCDPAGES;i++) {
        page = ( ( ( SSD1306_LCDPAGES - i ) - 1 ) * display->bufferwidth ) + display->xoff;
        SPI1_Exchange8bitBuffer(display->graphicBuffer + page, SSD1306_LCDWIDTH, NULL);
        //SPI1_Exchange8bitBuffer(graphicBuffer + page, SSD1306_LCDWIDTH, inbuffer);
    }

    PIN_DISPLAY_CS_HIGH;
    //spi1_close();
}



*/
