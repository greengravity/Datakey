/*
 * File:   buttons.c
 * Author: greengravity
 *
 * Created on 14. Januar 2023, 16:01
 */
#include <xc.h>
#include "buttons.h"
#include "mcc_ext.h"

#define UPDATE_PERIOD 156 // 5 millisecs

static uint8_t currButtonmap = 0x00;
static uint8_t prevButtonmap = 0x00;

//Mainfunction which makes indrect assignment of buttons to pins
bool readButtonDown(uint8_t button) {

    if (button == BUTTON_LEFT) {
        return BUTTON_LEFT_READ;
    }
    if (button == BUTTON_UP) {        
        return BUTTON_UP_READ;
    }
    if (button == BUTTON_RIGHT) {
        return BUTTON_RIGHT_READ;
    }
    if (button == BUTTON_DOWN) {        
        return BUTTON_DOWN_READ;        
    }
    if (button == BUTTON_A) {
        return BUTTON_A_READ;       
    }
    if (button == BUTTON_B) {
        return BUTTON_B_READ;       
    }

    return false;
}


void updateButtons(bool force) {
    prevButtonmap = currButtonmap;

    if ( buttonTimer == 0 || force ) {
        buttonTimer = 20;        
        currButtonmap = 0x00;
        for (int i=0;i< BUTTON_COUNT; i++) {
            if ( readButtonDown( i ) ) {  
                currButtonmap |= ( 0x01 << i );
            }       
        }
    }
}

bool isButtonDown(uint8_t button) {
    return ( ( currButtonmap & ( 0x01 << button ) ) > 0x00 );
}

bool isButtonUp(uint8_t button) {
    return ( ( currButtonmap & ( 0x01 << button ) ) == 0x00 );    
}

bool isButtonPressed(uint8_t button) {
    return ( isButtonDown( button ) && ( ( prevButtonmap & ( 0x01 << button ) ) == 0x00 ) );
}

bool isButtonReleased(uint8_t button) {
    return ( isButtonUp( button ) && ( ( prevButtonmap & ( 0x01 << button ) ) > 0x00 ) );    
}





