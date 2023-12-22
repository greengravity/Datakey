/* 
 * File:   assets_common.h
 * Author: greengravity
 *
 * Created on 20. Dezember 2023, 17:18
 */

#ifndef ASSETS_COMMON_H
#define	ASSETS_COMMON_H

#ifdef	__cplusplus
extern "C" {
#endif

#include	<xc.h>
#include	<stdbool.h>
 
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

uint8_t unicodeLookup(uint16_t ct);

#ifdef	__cplusplus
}
#endif

#endif	/* ASSETS_COMMON_H */

