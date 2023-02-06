#ifndef SYSIMAGES_H
#define	SYSIMAGES_H
#include	<xc.h>
#include	<stdbool.h>

#define MAX_IMAGE_BUFFER 225
#define TOTAL_IMAGE_COUNT 118
#define TOTAL_CHAR_COUNT 110
#define CHAR_HEIGHT 15
#define CHAR_EOL 0x00
#define CHAR_LINEFEED 0x01
#define CHAR_ENCODEERR 0x02
#define CHAR_SPACE 0x03

// bmpsize: 12736Bytes
typedef struct {
  const uint8_t width; // Bitmap width in pixels
  const uint8_t height; // Bitmap width in pixels
  const uint16_t bitmapOffset; // offset in bitmaparray
} GFXimage;

typedef struct {
  const uint16_t id; // Image id
  const uint8_t xoff;
  const uint8_t xadv;
  const uint16_t uccp; // Unicode codepoint
} GFXChar;

typedef struct {
  const uint16_t uccp;
  const uint8_t cid;
} Unicodelist;

typedef struct {
  const uint8_t active;
  const uint16_t name;
  const uint16_t layoutoff;
} Keyboardmaps;

typedef struct {
  const uint8_t fkt;
  const uint8_t id;
  const uint8_t span_pos;
  const uint8_t span_neg;
} Keylayout;

typedef struct {
  const uint16_t name;
  const uint16_t charset;
  const uint8_t len;
} Generatormaps;

extern const uint16_t bitmapdata[];
extern const GFXimage bitmaps[];
extern const GFXChar gfxchars[];
extern const Unicodelist unicodes[];
extern const Keyboardmaps keymaps[];
extern const Generatormaps generatormaps[];
extern const Keylayout keylayouts[];
extern const uint8_t textdata[];
extern const uint16_t texte[];
extern const uint8_t hexchars[];

#define KEYCOMMAND_EMPTY 0
#define KEYCOMMAND_CHAR 1
#define KEYCOMMAND_SPACE 2
#define KEYCOMMAND_BACKSPACE 3
#define KEYCOMMAND_LF 4
#define KEYCOMMAND_OK 5
#define KEYCOMMAND_ABORT 6
#define KEYCOMMAND_DEL 7
#define KEYCOMMAND_GEN 8

#define SYSICON_EMPTY 107
#define SYSICON_SPACETEXT 108
#define SYSICON_ENCODEERR 109
#define SYSICON_BACKSPACE 110
#define SYSICON_SPACE 111
#define SYSICON_ENTER 112
#define SYSICON_OK 113
#define SYSICON_DELETE 114
#define SYSICON_ABORT 115
#define SYSICON_ARROWUP 116
#define SYSICON_ARROWDOWN 117

#define TEXT_ERROR_CONTEXT 0
#define TEXT_ERROR_SD_CD 1
#define TEXT_ERROR_SD_FAILURE 2
#define TEXT_TEST 3
#define TEXT_ENTER_PIN 4
#define TEXT_ENTER_NEW_PIN1 5
#define TEXT_ENTER_NEW_PIN2 6
#define TEXT_ENTER_NEW_PIN_ERROR 7
#define TEXT_ENTER_NEW_KEY 8
#define TEXT_ENTER_NEW_KEY_ERROR 9
#define TEXT_IOHEAD_NEW 10
#define TEXT_IOHEAD_NAME 11
#define TEXT_IOHEAD_URL 12
#define TEXT_IOHEAD_KEY 13
#define TEXT_IOHEAD_KEY2 14
#define TEXT_IOHEAD_INFO 15
#define TEXT_VIEWHEAD_NAME 16
#define TEXT_VIEWHEAD_URL 17
#define TEXT_VIEWHEAD_KEY 18
#define TEXT_VIEWHEAD_KEY2 19
#define TEXT_VIEWHEAD_INFO 20
#define TEXT_HEAD_ENTRY_DETAIL 21
#define TEXT_HEAD_ENTRY_OVERVIEW 22
#define TEXT_EDETAIL_NAME 23
#define TEXT_EDETAIL_KEY1 24
#define TEXT_EDETAIL_KEY2 25
#define TEXT_EDETAIL_URL 26
#define TEXT_EDETAIL_INFO 27
#define TEXT_MSG_OVERWRITE 28
#define TEXT_MSG_DELETE_TOKEN 29
#define TEXT_MSG_DELETE_ENTRY 30
#define TEXT_MSG_ABORT 31
#define TEXT_LOAD_ENTRY_PLACEHOLDER 32

#define DEFAULT_KEYMAP 3

#define PASSWORD_REPLACE_CHAR 0x0d

#endif