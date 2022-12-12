#include <JoyStick.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_ADS1x15.h>

extern TwoWire i2cBus;
extern bool listHasError[5];

typedef enum
{
    JOYSTICK = 4,
} ERROR_INIT;

Adafruit_ADS1115 ads;
JoystickPin joystick;

void joystickSetup(uint8_t VRxPin, uint8_t VRyPin, uint8_t SWPin)
{
    buttonSetup(SWPin);
    joystick.X_channel = VRxPin; // adc channel 0
    joystick.Y_channel = VRyPin; // adc channel 1

    if (!ads.begin(0x48, &i2cBus))
    {
        Serial.println("ADS1115 not found");
        listHasError[JOYSTICK] = true;
    }
}
void joystickLoop()
{
    buttonLoop();
    joystick.JoystickValue[0] = ads.readADC_SingleEnded(joystick.X_channel);
    joystick.JoystickValue[1] = ads.readADC_SingleEnded(joystick.Y_channel);
}
JoystickAxisState joystickAxisReadState()
{
    JoystickAxisState state = AXIS_CENTER;
    if (joystick.JoystickValue[0] < 5000)
    {
        state = AXIS_LEFT;
    }
    else if (joystick.JoystickValue[0] > 10000)
    {
        state = AXIS_RIGHT;
    }
    else if (joystick.JoystickValue[1] < 5000)
    {
        state = AXIS_UP;
    }
    else if (joystick.JoystickValue[1] > 10000)
    {
        state = AXIS_DOWN;
    }
    return state;
}
ButtonState joystickButtonReadState()
{
    ButtonState state = buttonReadState();
    return state;
}