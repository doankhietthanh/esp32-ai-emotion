#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <ezButton.h>

typedef enum
{
    PRESSED_NONE = 0x00,
    PRESSED_SHORT = 0x01,
    PRESSED_LONG = 0x02,
} ButtonState;

void buttonSetup(uint8_t buttonPin);
void buttonLoop();
ButtonState buttonReadState();

#endif