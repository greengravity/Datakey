/*
 * File:   buttons.c
 * Author: greengravity
 *
 * Created on 14. Januar 2023, 16:01
 */
#include <xc.h>
#include "buttons.h"
#include "mcc_generated_files/tmr2.h"

#define UPDATE_PERIOD 156 // 5 millisecs

uint8_t currButtonmap = 0x00;
uint8_t prevButtonmap = 0x00;
uint32_t lastupdate = 0x00;

//Mainfunction which makes indrect assignment of buttons to pins
bool readButtonDown(uint8_t button) {

    if (button == BUTTON_LEFT) {
        return !BTN2_GetValue();
    }
    if (button == BUTTON_UP) {
        return !BTN1_GetValue();
    }
    if (button == BUTTON_RIGHT) {
        return !BTN5_GetValue();
    }
    if (button == BUTTON_DOWN) {
        return !BTN3_GetValue();
    }
    if (button == BUTTON_A) {
        return !BTN4_GetValue();
    }
    if (button == BUTTON_B) {
        return !BTN6_GetValue();
    }

    
/*    if (button == BUTTON_LEFT) {
        return !BTN1_GetValue();
    }
    if (button == BUTTON_UP) {
        return !BTN2_GetValue();
    }
    if (button == BUTTON_RIGHT) {
        return !BTN3_GetValue();
    }
    if (button == BUTTON_DOWN) {
        return !BTN4_GetValue();
    }
    if (button == BUTTON_A) {
        return !BTN5_GetValue();
    }
    if (button == BUTTON_B) {
        return !BTN6_GetValue();
    }    
*/    
    return false;
}


void updateButtons(bool force) {
    uint32_t time = TMR2_Counter32BitGet();
    prevButtonmap = currButtonmap;
    if ( time > ( lastupdate + UPDATE_PERIOD ) || force ) {
        lastupdate = time;                
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





