/* 
 * File:   glyphs.h
 * Author: greengravity
 *
 * Created on 25. März 2018, 10:03
 */

#ifndef SYSIMAGES_H
#define	SYSIMAGES_H
#include <xc.h>
#include "systemfont.h"

#define SYSTEMSYMBOLS 0
#define KEYBOARDPATTERNSYMBOLS 1
#define SYSTEMFONT 0

#define SSYMB_KEYBOARD_DEL_SYMBOL 0
#define SSYMB_KEYBOARD_CURSOR_BACKWARD_SYMBOL 1
#define SSYMB_KEYBOARD_CURSOR_FORWARD_SYMBOL 2
#define SSYMB_KEYBOARD_BACK_SYMBOL 3
#define SSYMB_KEYBOARD_ACCEPT_SYMBOL 4
#define SSYMB_KEYBOARD_RETURN_SYMBOL 5

#define SSYMB_INPUTFIELD_CURSOR_SYMBOL 6
#define SSYMB_INPUTFIELD_ID_SYMBOL  7
#define SSYMB_INPUTFIELD_DATA_SYMBOL 8

#define SSYMB_FUNCTIONIMG_VIEWKEY 9
#define SSYMB_FUNCTIONIMG_ADDKEY 10
#define SSYMB_FUNCTIONIMG_CONFIG 11
#define SSYMB_FUNCTIONIMG_LEFT 12
#define SSYMB_FUNCTIONIMG_RIGHT 13
#define SSYMB_FUNCTIONIMG_UP 14
#define SSYMB_FUNCTIONIMG_DOWN 15
#define SSYMB_FUNCTIONIMG_KBHIT 16
#define SSYMB_FUNCTIONIMG_BACK 17

#define SSYMB_SMALLNUMBER_START  18 
#define SSYMB_SMALLNUMBER_SPLIT  34 

#define MAX_IMAGE_BUFFER 1024

//#define SYSFONT_NUMBER_START 15 
//#define SYSFONT_MINUS 12

typedef struct { // Data stored PER GLYPH
  const uint8_t width; // Bitmap width in pixels
  const uint8_t height; // Bitmap width in pixels
  const uint8_t xoff;
  const uint8_t xadv;
  const uint16_t bitmapOffset;     // Pointer into GFXfont->bitmap  
} GFXimage;

typedef struct { // Data stored for FONT AS A WHOLE:  
  const uint8_t  *bitmap;  // image bitmaps, concatenated
  const GFXimage *image; // Image array
  const uint16_t images; // Amount of Glyphs in this Font  
} GFXfont;

typedef struct { // Data stored for FONT AS A WHOLE:  
  const uint8_t  *bitmap;  // image bitmaps, concatenated
  const GFXimage *image; // Image array
  const uint16_t images; // Amount of Images in this list
} GFXimagelist;



extern const int sizeofimage;

/*Helper function to get a sortable value for a charactervalue*/
/*To make sort case unsensitive */
extern int getEntryIndexTextCompareValue(uint8_t cval);

/************** BEGIN OF BITMAPS **********************/
/**************** Character bitmaps and describers for Systemmenu **************/
extern const uint8_t systemsymb_Bitmaps[];
extern const GFXimage systemsymb_Descriptors[];

//Character bitmaps and describers for standard Systemfont
extern const uint8_t systemfont_Bitmaps[];
extern const GFXimage systemfont_Descriptors[];

//Character bitmaps and describers for the keyboardlayout (images for patternreferences)
extern const uint8_t keyboardpatternsymb_Bitmaps[];
extern const GFXimage keyboardpatternsymb_Descriptors[];

/************** FONT SUMMARY ****************************/
extern const GFXfont gfxfonts[];
extern const GFXimagelist gfximages[];

#endif	/* GLYPHS_H */

