#include <JoyStick.h>

JoystickPin joystick;

void joystickSetup(uint8_t VRxPin, uint8_t VRyPin, uint8_t SWPin)
{
    buttonSetup(SWPin);
    joystick.X_channel = VRxPin;
    joystick.Y_channel = VRyPin;
}
void joystickLoop()
{
    buttonLoop();
    joystick.JoystickValue[0] = analogRead(joystick.X_channel);
    joystick.JoystickValue[1] = analogRead(joystick.Y_channel);
}
JoystickAxisState joystickAxisReadState()
{
    JoystickAxisState state = JOYSTICK_TYPE_CENTER;
    if (joystick.JoystickValue[0] < 1000)
    {
        state = JOYSTICK_TYPE_LEFT;
    }
    else if (joystick.JoystickValue[0] > 2500)
    {
        state = JOYSTICK_TYPE_RIGHT;
    }
    else if (joystick.JoystickValue[1] < 1000)
    {
        state = JOYSTICK_TYPE_UP;
    }
    else if (joystick.JoystickValue[1] > 2500)
    {
        state = JOYSTICK_TYPE_DOWN;
    }
    return state;
}
ButtonState joystickButtonReadState()
{
    ButtonState state = buttonReadState();
    return state;
}