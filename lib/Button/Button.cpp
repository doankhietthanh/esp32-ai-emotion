#include <Button.h>
#include <Arduino.h>

uint16_t LONG_PRESS_TIME = 2000;  // 1 second
uint16_t SHORT_PRESS_TIME = 1000; // 100 ms

unsigned long pressedTime = 0;
unsigned long releasedTime = 0;
bool isPressing = false;
bool isLongDetected = false;

ezButton *ezbtn;

void buttonSetup(uint8_t buttonPin)
{
    ezbtn = new ezButton(buttonPin);
}

void buttonLoop()
{
    ezbtn->loop();
}

ButtonState buttonReadState()
{
    if (ezbtn->isPressed())
    {
        pressedTime = millis();
        isPressing = true;
        isLongDetected = false;
    }

    if (ezbtn->isReleased())
    {
        isPressing = false;
        releasedTime = millis();

        long pressDuration = releasedTime - pressedTime;

        if (pressDuration < SHORT_PRESS_TIME)
        {
            return BUTTON_TYPE_SHORT;
        }
    }

    if (isPressing == true && isLongDetected == false)
    {
        long pressDuration = millis() - pressedTime;

        if (pressDuration > LONG_PRESS_TIME)
        {
            isLongDetected = true;
            return BUTTON_TYPE_LONG;
        }
    }

    return BUTTON_TYPE_NONE;
}
