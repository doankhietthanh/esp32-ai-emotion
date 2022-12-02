#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#include <Arduino.h>
#include <Button.h>

typedef enum
{
    JOYSTICK_TYPE_CENTER = 0x00,
    JOYSTICK_TYPE_UP = 0x01,
    JOYSTICK_TYPE_DOWN = 0x02,
    JOYSTICK_TYPE_LEFT = 0x03,
    JOYSTICK_TYPE_RIGHT = 0x04,
} JoystickAxisState;

typedef struct
{
    uint8_t X_channel;
    uint8_t Y_channel;
    uint32_t JoystickValue[2];
} JoystickPin;

void joystickSetup(uint8_t VRxPin, uint8_t VRyPin, uint8_t SWPin);
void joystickLoop();
JoystickAxisState joystickAxisReadState();
ButtonState joystickButtonReadState();

#endif