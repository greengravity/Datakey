/* 
 * File:   
 * Author: greengravity
 * Comments:
 * Revision history: 
 */

#ifndef XC_LOGIC_H
#define	XC_LOGIC_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdbool.h>
#include "fs/ff.h"
#include "assets.h"

#define CTX_BUFFER_SIZE 2000
#define PIN_SIZE 6
#define MAX_PIN_TRIES 6

#define MAX_TOKEN_
#define MAX_TEXT_LEN 1023
#define TEXTAREA_WIDTH 150
#define TEXTAREA_VIEWPORT_LINES 3
#define TEXTVIEWER_LINES 6
#define OVERVIEW_LINES 6
#define MAX_TEXTAREA_SPACE_BREAK 80
//#define TEXTAREA_RINGBUFFER_LEN 64
#define TOKEN_BLOCK_SIZE 4096  //256*16 
#define TOKEN_READ_MOD_BUFFERSIZE 512 //TOKEN_BLOCK_SIZE % TOKEN_READ_MOD_BUFFERSIZE must be zero
#define MAX_OVERVIEW_ENTRY_COUNT 24
#define OVERVIEW_RELOAD_OFFS (MAX_OVERVIEW_ENTRY_COUNT / 2 - (OVERVIEW_LINES + 1))


typedef enum {
    NEW,
    NAME,
    KEY1,
    KEY2,
    URL,                
    INFO,
    CHECK,
} TOKEN_TYPE;

typedef enum {
    USB_MODE_OFF,
    USB_MODE_KEYBOARD          
} USB_MODE;

typedef struct {
    uint16_t highlight_color1;
    uint16_t highlight_color2;
    uint8_t brightness;
    USB_MODE umode;
} DEVICE_OPTIONS;

typedef struct {
    TOKEN_TYPE type;
    uint8_t textid; 
    uint8_t textidview; 
} TOKEN_CONFIG;

typedef enum {    
    UNCHANGED,
    REFRESH,    
    ONBUTTON_PRESS,
    ONBUTTON_RELEASE,
    ONBUTTON_DOWN,
    REMOVECHAR,
    ANIMATION,
    LOADENTRY,
    AREASWITCH,
    IO_UPDATE,
} RENDER_INFO;

typedef enum {
    EMPTY,
    ERROR,
    ERROR_CONTEXT,
    ERROR_SD_CD,
    ERROR_SD_FAILURE,
    INITIAL,
    PIN_INPUT, 
    KEY_INPUT, 
    ENTRY_OVERVIEW,
    ENTRY_DETAIL,
    VIEW_TOKEN,
    EDIT_ENTRY,
    MESSAGEBOX,
} CONTEXT_TYPE;


typedef enum {  
    MSGB_NO_RESULT = 0,
    MSGB_YES = 1,
    MSGB_NO = 2,
} MSGB_RESULT;

typedef enum {
    YES_NO,    
} MSGTYPE;


typedef struct {
    char msg[256];
} CTX_ERROR;

typedef struct {    
    RENDER_INFO rinf;
    TOKEN_TYPE type;    
    MSGB_RESULT mboxresult;
    uint8_t datachanged;
    uint8_t okbmap;   //old keymap
    uint8_t kbmap;    //curr keymap
    uint8_t okbx;     //old keyx - for animation
    uint8_t okby;     //old keyy - for animation  
    uint8_t kbx;      //keyx
    uint8_t kby;      //keyy
    uint8_t oselarea; //old working area 
    uint8_t selarea;  //current working area
    uint8_t tewpx;    //textarea current writer x position
    uint8_t text[MAX_TEXT_LEN + 17];
    uint8_t __pad1;
    uint16_t tavccoff;  //textarea viewport current character offset
    uint16_t tavcloff;  //textarea viewport current lineoffset
    uint16_t tewp;    //textarea current writer position
    uint16_t tlen;    //current length of text        
} IO_CONTEXT;


//Context for Pin Input
typedef struct {
    bool generatenew;
    bool verify;
    bool error;
    bool haderror;
    uint8_t ppos;
    uint8_t __pad1;
    uint8_t pin[PIN_SIZE];
    uint8_t verifypin[PIN_SIZE];        
} CTX_PIN_INPUT;

typedef struct { 
    bool error;
    bool haderror;
    uint8_t kpos;
    uint8_t x;
    uint8_t y;
    uint8_t oldx;
    uint8_t oldy;
    uint8_t __pad1;
    uint8_t newkey[32];    
    uint8_t charlocations[64];
} CTX_KEY_INPUT;

typedef struct {
    bool loaded;
    TCHAR path[16];
    uint8_t name[48];
    uint8_t __pad1;
} CTX_OVERVIEW_ENTRY;


typedef struct {        
    bool initialized;
    uint8_t ohighlightpos;
    CTX_OVERVIEW_ENTRY entries[MAX_OVERVIEW_ENTRY_COUNT];    
    uint16_t cursor;
    uint16_t bufferstart;
    uint16_t bufferlen;
    uint16_t entrycenter;
    uint16_t entrycount;
} CTX_ENTRY_OVERVIEW;

typedef struct {    
    TCHAR path[16];
    TOKEN_TYPE selected;
    TOKEN_TYPE oselected;
    uint8_t name[80];
    uint8_t key1[32];
    uint8_t key2[32];
    uint8_t url[80];
    uint8_t info[80];
    uint16_t overviewcursor;
} CTX_ENTRY_DETAIL;


typedef struct {    
    IO_CONTEXT io;
    TCHAR path[16];
    uint16_t overviewcursor;
} CTX_EDIT_ENTRY;

typedef struct {    
    uint8_t text[MAX_TEXT_LEN + 17];
    TOKEN_TYPE type;
    uint8_t __pad1;
    uint16_t tlen;
    uint16_t currline;
    uint16_t lines;
} CTX_VIEW_TOKEN;

typedef struct {
    uint8_t msgtxt[50];
    MSGB_RESULT res;
    MSGTYPE mtype;
} CTX_MSG_BOX;

//Context storage
typedef struct {
    uint16_t ctxptr; 
    uint16_t bufferlen;
    uint8_t *ctxbuffer;
    uint8_t ctxtype;
    RENDER_INFO rinf;
    uint8_t power;   
    bool fsmounted;
    bool fileopen;
    uint8_t __pad1;
    FATFS drive;
    FIL file;
} APP_CONTEXT;

extern const TOKEN_CONFIG token_configs[];
extern DEVICE_OPTIONS device_options;

bool isKeySet( );
void setMasterKey(uint8_t *key);
uint8_t* getMasterKey();
void swipeKeys();
uint8_t verifyMasterKey();

uint8_t getCharactersInLine(uint8_t* text, uint16_t coff, uint16_t maxchars );
bool setContext(APP_CONTEXT *ctx, CONTEXT_TYPE type);
void setInitialContext( APP_CONTEXT* ctx );
void updateContext(APP_CONTEXT* ctx);

#endif	/* XC_HEADER_TEMPLATE_H */

