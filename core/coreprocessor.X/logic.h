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





typedef enum {
    NEW,
    NAME,
    KEY,
    KEY2,
    URL,                
    INFO,
} TOKEN_TYPE;

typedef struct {
    TOKEN_TYPE type;
    uint8_t textid;                
} TOKEN_CONFIG;

typedef enum  {    
    UNCHANGED,
    REFRESH,    
    ONBUTTON_PRESS,
    ONBUTTON_RELEASE,
    ONBUTTON_DOWN,
    REMOVECHAR,
    ANIMATION,
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
    NEW_ENTRY,
} CONTEXT_TYPE;

typedef struct {
    char msg[256];
} CTX_ERROR;

typedef struct {    
    RENDER_INFO rinf;
    TOKEN_TYPE type;    
    uint8_t kbmap;
    uint8_t okbx;  //old Keyx - for animation
    uint8_t okby;  //old Keyy - for animation  
    uint8_t kbx;   //Keyx
    uint8_t kby;   //Keyy
    uint8_t selarea;   
    uint8_t tacline; //textarea current line
    uint8_t tecx;    //textarea current x position
    char text[1024];
    uint16_t lineoffsets[128]; //characteroffset at each linestart 
    uint16_t tecc;    //textarea current character position
    uint16_t tlen;    //current length of text
} IO_CONTEXT;

//Context storage
typedef struct {
    uint16_t ctxptr; 
    uint16_t bufferlen;
    uint8_t *ctxbuffer;
    uint8_t ctxtype;
    RENDER_INFO rinf;
    uint8_t power;
    bool fsmounted;
    FATFS drive;
    bool fileopen;
    FIL file;
} APP_CONTEXT;


//Context for Pin Input
typedef struct {
    bool generatenew;
    bool verify;
    bool error;
    bool haderror;
    uint8_t pin[PIN_SIZE];
    uint8_t verifypin[PIN_SIZE];    
    uint8_t ppos;    
} CTX_PIN_INPUT;

typedef struct { 
    bool error;
    bool haderror;
    uint8_t newkey[32];
    uint8_t kpos;
    uint8_t x;
    uint8_t y;
    uint8_t oldx;
    uint8_t oldy;
    uint8_t charlocations[64];
} CTX_KEY_INPUT;

typedef struct {    
    uint32_t written;
} CTX_ENTRY_OVERVIEW;

typedef struct {    
    IO_CONTEXT io;    
} CTX_NEW_ENTRY;

extern const TOKEN_CONFIG token_configs[];


bool isKeySet( );
void setMasterKey(uint8_t *key);
uint8_t* getMasterKey();
void swipeKeys();
uint8_t verifyMasterKey();

bool setContext(APP_CONTEXT *ctx, CONTEXT_TYPE type);
void setInitialContext( APP_CONTEXT* ctx );
void updateContext(APP_CONTEXT* ctx);

#endif	/* XC_HEADER_TEMPLATE_H */

