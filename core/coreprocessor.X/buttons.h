/* 
 * File:   
 * Author: greengravity
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef XC_BUTTONS_H
#define	XC_BUTTONS_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include "mcc_generated_files/pin_manager.h"
#include <stdbool.h>

#define BUTTON_LEFT 0
#define BUTTON_UP 1
#define BUTTON_RIGHT 2
#define BUTTON_DOWN 3
#define BUTTON_A 4
#define BUTTON_B 5

#define BUTTON_COUNT 6

/*
 * Breadboard Config
#define BUTTON_LEFT_READ (!BTN2_GetValue())
#define BUTTON_UP_READ (!BTN3_GetValue())
#define BUTTON_RIGHT_READ (!BTN5_GetValue())
#define BUTTON_DOWN_READ (!BTN1_GetValue())
#define BUTTON_A_READ (!BTN4_GetValue())
#define BUTTON_B_READ (!BTN6_GetValue())
*/

/*
 * Production Config
#define BUTTON_LEFT_READ (!BTN1_GetValue())
#define BUTTON_UP_READ (!BTN3_GetValue())
#define BUTTON_RIGHT_READ (!BTN4_GetValue())
#define BUTTON_DOWN_READ (!BTN2_GetValue())
#define BUTTON_A_READ (!BTN6_GetValue())
#define BUTTON_B_READ (!BTN5_GetValue())
*/

/* 
 * Test Config 
 */
#define BUTTON_LEFT_READ (!BTN4_GetValue())
#define BUTTON_UP_READ (!BTN2_GetValue())
#define BUTTON_RIGHT_READ (!BTN1_GetValue())
#define BUTTON_DOWN_READ (!BTN3_GetValue())
#define BUTTON_A_READ (!BTN6_GetValue())
#define BUTTON_B_READ (!BTN5_GetValue())


void updateButtons(bool force);
bool isButtonPressed(uint8_t button);
bool isButtonReleased(uint8_t button);
bool isButtonDown(uint8_t button);
bool isButtonUp(uint8_t button);

#endif	/* XC_HEADER_TEMPLATE_H */

