#ifndef SYSIMAGES_H
#define	SYSIMAGES_H
#include	<xc.h>
#include	<stdbool.h>

#define MAX_IMAGE_BUFFER 240
#define TOTAL_IMAGE_COUNT 111
#define TOTAL_CHAR_COUNT 109
#define CHAR_HEIGHT 14
#define ENCODING_ERROR_CHAR 0

// bmpsize: 12988Bytes
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
  const uint16_t uccp; // Image id
  const uint8_t cid;
} Unicodelist;

typedef struct {
  const uint8_t active;
  const uint8_t name[5];
  const uint16_t layoutoff;
} Keyboardmaps;

typedef struct {
  const uint8_t fkt;
  const uint8_t id;
  const uint8_t span_pos;
  const uint8_t span_neg;
} Keylayout;

extern const uint16_t bitmapdata[];
extern const GFXimage bitmaps[];
extern const GFXChar gfxchars[];
extern const Unicodelist unicodes[];
extern const Keyboardmaps keymaps[];
extern const Keylayout keylayouts[];

#define KEYCOMMAND_CHAR 0
#define KEYCOMMAND_SPACE 1
#define KEYCOMMAND_DEL 2
#define KEYCOMMAND_OK 3
#define KEYCOMMAND_BACK 4
#define KEYCOMMAND_EMPTY 5

#define SYSICON_DELETE 108
#define SYSICON_SPACE 109
#define SYSICON_OK 110

#endif