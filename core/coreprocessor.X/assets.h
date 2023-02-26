#ifndef SYSIMAGES_H
#define	SYSIMAGES_H
#include	<xc.h>
#include	<stdbool.h>

#define MAX_IMAGE_BUFFER 225
#define TOTAL_IMAGE_COUNT 151
#define TOTAL_CHAR_COUNT 111
#define CHAR_HEIGHT 15
#define CHAR_EOL 0x00
#define CHAR_LINEFEED 0x01
#define CHAR_ENCODEERR 0x02
#define CHAR_SPACE 0x03

// bmpsize: 24899Bytes
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
  const uint8_t scancode[4]; // Keyboard scancode data
} GFXChar;

typedef struct {
  const uint16_t uccp;
  const uint8_t cid;
} Unicodelist;

typedef struct {
  const uint8_t active;
  const uint16_t name;
  const uint8_t defx;
  const uint8_t defy;
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

extern const __prog__ uint8_t __attribute__((space(prog))) bitmapdata[];
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
#define KEYCOMMAND_GEN 7

#define SYSICON_EMPTY 108
#define SYSICON_SPACETEXT 109
#define SYSICON_ENCODEERR 110
#define SYSICON_BACKSPACE 111
#define SYSICON_SPACE 112
#define SYSICON_ENTER 113
#define SYSICON_OK 114
#define SYSICON_DELETE 115
#define SYSICON_ABORT 116
#define SYSICON_ARROWUP 117
#define SYSICON_ARROWDOWN 118
#define SYSICON_BRIGHTNESS 119
#define SYSICON_USB 120
#define SYSICON_COLOR 121
#define SYSICON_CLOCK 122
#define SYSICON_CALENDAR 123
#define SYSICON_COLON 124
#define SYSICON_DOT 125
#define SYSICON_SPEED 126
#define SYSICON_CHARGE 127
#define SYSICON_NOCHARGE 128
#define SYSICON_CHARGING 129
#define SYSICON_CHARGED 130
#define SYSICON_PIN_MARKER 131
#define SYSICON_PIN_GROUND 132
#define SYSICON_PIN_OBS1 133
#define SYSICON_PIN_OBS2 134
#define SYSICON_PIN_OBS3 135
#define SYSICON_PIN_OBS4 136
#define SYSICON_PIN_OBS5 137
#define SYSICON_PIN_OBS6 138
#define SYSICON_PIN_OBS7 139
#define SYSICON_PIN_OBS8 140
#define SYSICON_PIN_OBS9 141
#define SYSICON_TETRIS_BG 142
#define SYSICON_TETRIS_WALL 143
#define SYSICON_TETRIS_I 144
#define SYSICON_TETRIS_J 145
#define SYSICON_TETRIS_L 146
#define SYSICON_TETRIS_O 147
#define SYSICON_TETRIS_S 148
#define SYSICON_TETRIS_T 149
#define SYSICON_TETRIS_Z 150

#define TEXT_EMPTY 0
#define TEXT_ERROR_CONTEXT 1
#define TEXT_ERROR_SD_CD 2
#define TEXT_ERROR_SD_FAILURE 3
#define TEXT_TEST 4
#define TEXT_ENTER_NEW_KEY 5
#define TEXT_ENTER_NEW_KEY_ERROR 6
#define TEXT_IOHEAD_NEW 7
#define TEXT_IOHEAD_NAME 8
#define TEXT_IOHEAD_USER 9
#define TEXT_IOHEAD_URL 10
#define TEXT_IOHEAD_KEY 11
#define TEXT_IOHEAD_KEY2 12
#define TEXT_IOHEAD_INFO 13
#define TEXT_VIEWHEAD_NAME 14
#define TEXT_VIEWHEAD_USER 15
#define TEXT_VIEWHEAD_URL 16
#define TEXT_VIEWHEAD_KEY 17
#define TEXT_VIEWHEAD_KEY2 18
#define TEXT_VIEWHEAD_INFO 19
#define TEXT_TKNAME_NAME 20
#define TEXT_TKNAME_USER 21
#define TEXT_TKNAME_KEY1 22
#define TEXT_TKNAME_KEY2 23
#define TEXT_TKNAME_URL 24
#define TEXT_TKNAME_INFO 25
#define TEXT_HEAD_ENTRY_DETAIL 26
#define TEXT_HEAD_ENTRY_OVERVIEW 27
#define TEXT_HEAD_OPTIONS 28
#define TEXT_HEAD_PINOPTIONS 29
#define TEXT_OPTV_USB_OFF 30
#define TEXT_OPTV_USB_KEYBOARD 31
#define TEXT_OPTV_QKEYS_OFF 32
#define TEXT_OPTV_QKEYS_ON 33
#define TEXT_MSG_OVERWRITE 34
#define TEXT_MSG_DELETE_TOKEN 35
#define TEXT_MSG_DELETE_ENTRY 36
#define TEXT_MSG_ABORT 37
#define TEXT_CHB_CREATE_ENTRY 38
#define TEXT_CHB_DELETE_ENTRY 39
#define TEXT_CHB_OPTIONS 40
#define TEXT_CHB_PINOPTIONS 41
#define TEXT_CHB_GAMES 42
#define TEXT_CHB_RETURN_OVERVIEW 43
#define TEXT_CHB_EDIT_TOKEN 44
#define TEXT_CHB_DELETE_TOKEN 45
#define TEXT_CHB_PUSH_TOKEN 46
#define TEXT_WRITE_USB_OUT 47
#define TEXT_LOAD_ENTRY_PLACEHOLDER 48
#define TEXT_PIN_INF_LEVEL 49
#define TEXT_PIN_INF_LEVELNR 50
#define TEXT_PIN_INF_STEPS 51
#define TEXT_PIN_INF_LIVES 52
#define TEXT_PIN_INF_NO_STEPS 53
#define TEXT_PIN_INF_SPLIT_STEPS 54
#define TEXT_PIN_INF_GE_ENTER_PIN 55
#define TEXT_PIN_INF_GE_VERIFY_PIN 56
#define TEXT_PIN_INF_GE_PIN_ERROR 57
#define TEXT_OPT_CHANGE_PIN 58
#define TEXT_OPT_PIN_TRIES 59
#define TEXT_OPT_PIN_LEN 60
#define TEXT_OPTINF_PIN_LEN_CHANGED 61
#define TEXT_TETRIS_GAME_OVER1 62
#define TEXT_TETRIS_GAME_OVER2 63
#define TEXT_TETRIS_GAME_START1 64
#define TEXT_TETRIS_GAME_START2 65
#define TEXT_TETRIS_SCORE 66
#define TEXT_TETRIS_LINES 67
#define TEXT_TETRIS_LEVEL 68

#define DEFAULT_KEYMAP 3

#define PASSWORD_REPLACE_CHAR 0x0d

#endif