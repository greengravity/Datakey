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
#include "mcc_generated_files/fatfs/ff.h"

#define CTX_BUFFER_SIZE 4000
#define PIN_SIZE 6
#define MAX_PIN_TRIES 6

typedef enum  {    
    UNCHANGED,
    REFRESH,    
    ONBUTTON_PRESS,
    ONBUTTON_RELEASE,
    ONBUTTON_DOWN,
    REMOVECHAR,
    ANIMATION,
    SPECIAL_FUNCTION
} RENDER_INFO;



typedef enum {
    ERROR,
    ERROR_CONTEXT,
    ERROR_SD_CD,
    ERROR_SD_FAILURE,
    INITIAL,
    PIN_INPUT, 
    KEY_INPUT, 
    KEY_OVERVIEW,
} CONTEXT_TYPE;

typedef struct {
    char msg[256];
} CTX_ERROR;

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
    bool render;
} CTX_KEY_OVERVIEW;



bool isKeySet( );
void setMasterKey(uint8_t *key);
uint8_t* getMasterKey();
void swipeKeys();
uint8_t verifyMasterKey();

void setInitialContext( APP_CONTEXT* ctx );
void updateContext(APP_CONTEXT* ctx);

#endif	/* XC_HEADER_TEMPLATE_H */

