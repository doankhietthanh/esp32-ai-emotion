#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <ezButton.h>

typedef enum
{
    BUTTON_TYPE_NONE = 0x00,
    BUTTON_TYPE_SHORT = 0x01,
    BUTTON_TYPE_LONG = 0x02,
} ButtonState;

void buttonSetup(uint8_t buttonPin);
void buttonLoop();
ButtonState buttonReadState();

#endif