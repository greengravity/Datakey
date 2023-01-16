/*
 * File:   buttons.c
 * Author: greengravity
 *
 * Created on 14. Januar 2023, 16:01
 */
#include <xc.h>
#include "buttons.h"

void updateButtons() {

}

bool isButtonPressed(uint8_t button) {

    return false;
}

bool isButtonReleased(uint8_t button) {

    return false;
}

bool isButtonDown(uint8_t button) {

    if (button == BUTTON_LEFT) {
        return !IO_RA9_GetValue();
    }
    if (button == BUTTON_UP) {
        return !IO_RA8_GetValue();
    }
    if (button == BUTTON_RIGHT) {
        return !IO_RA2_GetValue();
    }
    if (button == BUTTON_DOWN) {
        return !IO_RC2_GetValue();
    }
    if (button == BUTTON_A) {
        return !IO_RC1_GetValue();
    }
    if (button == BUTTON_B) {
        return !IO_RC0_GetValue();
    }


    return false;
}

bool isButtonUp(uint8_t button) {

    return false;
}

