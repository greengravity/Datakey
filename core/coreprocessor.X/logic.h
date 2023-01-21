
/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

#ifndef XC_LOGIC_H
#define	XC_LOGIC_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdbool.h>

#define CTX_BUFFER_SIZE 4000
#define PIN_SIZE 6

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
    INITIAL,
    PIN_INPUT, 
    KEY_INPUT, 
    KEY_OVERVIEW,
} CONTEXT_TYPE;


//Context storage
typedef struct {    
    uint16_t ctxptr; 
    uint16_t bufferlen;
    uint8_t *ctxbuffer;
    uint8_t ctxtype;
    RENDER_INFO rinf;
} APP_CONTEXT;


//Context for Pin Input
typedef struct {
    bool generatenew;
    uint8_t pin[PIN_SIZE];
    uint8_t ppos;    
} CTX_PIN_INPUT;

typedef struct {    
    uint8_t newkey[32];
    uint8_t kpos;
    uint8_t x;
    uint8_t y;
    uint8_t oldx;
    uint8_t oldy;
} CTX_KEY_INPUT;

typedef struct {
    bool render;
} CTX_KEY_OVERVIEW;



void setInitialContext( APP_CONTEXT* ctx );
void updateContext(APP_CONTEXT* ctx);

#endif	/* XC_HEADER_TEMPLATE_H */

