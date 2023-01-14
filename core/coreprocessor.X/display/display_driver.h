/* 
 * File:   display.h
 * Author: Ronald
 *
 * Created on March 23, 2018, 12:11 PM
 */

#ifndef DISPLAY_DRIVER_H
#define	DISPLAY_DRIVER_H
    
#include "../main.h"
#include "../mcc_generated_files/system.h"
#include "../spi_ext.h"
#include "../mcc_generated_files/spi1_driver.h"
#include "../mcc_generated_files/pin_manager.h"
#include "../images/sysimages.h"

#define ST77XX_PIN_CS_HIGH   IO_RC8_SetHigh()
#define ST77XX_PIN_CS_LOW    IO_RC8_SetLow()
#define ST77XX_PIN_DC_HIGH  IO_RC9_SetHigh()
#define ST77XX_PIN_DC_LOW    IO_RC9_SetLow()
#define ST77XX_PIN_RES_HIGH  IO_RB13_SetHigh()
#define ST77XX_PIN_RES_LOW   IO_RB13_SetLow()
#define ST77XX_PIN_LED_HIGH  IO_RB8_SetHigh()
#define ST77XX_PIN_LED_LOW   IO_RB8_SetLow()

#define ST77XX_NOP 0x00
#define ST77XX_SWRESET 0x01
#define ST77XX_RDDID 0x04
#define ST77XX_RDDST 0x09

#define ST77XX_SLPIN 0x10
#define ST77XX_SLPOUT 0x11
#define ST77XX_PTLON 0x12
#define ST77XX_NORON 0x13

#define ST77XX_INVOFF 0x20
#define ST77XX_INVON 0x21
#define ST77XX_DISPOFF 0x28
#define ST77XX_DISPON 0x29
#define ST77XX_CASET 0x2A
#define ST77XX_RASET 0x2B
#define ST77XX_RAMWR 0x2C
#define ST77XX_RAMRD 0x2E

#define ST77XX_PTLAR 0x30
#define ST77XX_TEOFF 0x34
#define ST77XX_TEON 0x35
#define ST77XX_MADCTL 0x36
#define ST77XX_COLMOD 0x3A

#define ST77XX_MADCTL_MY 0x80
#define ST77XX_MADCTL_MX 0x40
#define ST77XX_MADCTL_MV 0x20
#define ST77XX_MADCTL_ML 0x10
#define ST77XX_MADCTL_RGB 0x00

#define ST77XX_RDID1 0xDA
#define ST77XX_RDID2 0xDB
#define ST77XX_RDID3 0xDC
#define ST77XX_RDID4 0xDD

// Some ready-made 16-bit ('565') color settings:
#define ST77XX_BLACK 0x0000
#define ST77XX_WHITE 0xFFFF
#define ST77XX_RED 0xF800
#define ST77XX_GREEN 0x07E0
#define ST77XX_BLUE 0x001F
#define ST77XX_CYAN 0x07FF
#define ST77XX_MAGENTA 0xF81F
#define ST77XX_YELLOW 0xFFE0
#define ST77XX_ORANGE 0xFC00



#define INITR_GREENTAB 0x00
#define INITR_REDTAB 0x01
#define INITR_BLACKTAB 0x02
#define INITR_18GREENTAB INITR_GREENTAB
#define INITR_18REDTAB INITR_REDTAB
#define INITR_18BLACKTAB INITR_BLACKTAB
#define INITR_144GREENTAB 0x01
#define INITR_MINI160x80 0x04
#define INITR_HALLOWING 0x05

// Some register settings
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MH 0x04

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR 0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1 0xC0
#define ST7735_PWCTR2 0xC1
#define ST7735_PWCTR3 0xC2
#define ST7735_PWCTR4 0xC3
#define ST7735_PWCTR5 0xC4
#define ST7735_VMCTR1 0xC5

#define ST7735_PWCTR6 0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// Some ready-made 16-bit ('565') color settings:
#define ST7735_BLACK ST77XX_BLACK
#define ST7735_WHITE ST77XX_WHITE
#define ST7735_RED ST77XX_RED
#define ST7735_GREEN ST77XX_GREEN
#define ST7735_BLUE ST77XX_BLUE
#define ST7735_CYAN ST77XX_CYAN
#define ST7735_MAGENTA ST77XX_MAGENTA
#define ST7735_YELLOW ST77XX_YELLOW
#define ST7735_ORANGE ST77XX_ORANGE


#define ST7735_TFTWIDTH 160  // for 1.44 and mini
#define ST7735_TFTHEIGHT 128 // for 1.8" and mini display

#define ST_CMD_DELAY 0x80 // special signifier for command lists


void setupDisplay( );
void writeDisplay();

void drawPixel(int16_t x, int16_t y, uint16_t color);
void fillRect(int16_t x, int16_t y, int16_t x2, int16_t y2, uint16_t color);
void clearScreen(uint16_t color);
void drawImage(int16_t x, int16_t y, uint16_t colorvg, uint16_t colorbg, const GFXimage *image, const uint8_t *bitmap);


#endif	/* DISPLAY_DRIVER_H */






