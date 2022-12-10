#include <Arduino.h>
#include <Joystick.h>
#include <Wire.h>

const int VRxPin = 0;
const int VRyPin = 1;
const int SWPin = 2;

const int SDAPin = 14;
const int SCLPin = 15;

int VRx = 0; // value read from the horizontal pot
int VRy = 0; // value read from the vertical pot
int SW = 0;  // value read from the swistch
int stateJoystick = 0;

TwoWire i2cBus = TwoWire(0);

void setup()
{
  Serial.begin(115200);
  i2cBus.begin(SDAPin, SCLPin, 400000);
  joystickSetup(VRxPin, VRyPin, SWPin);
}

void loop()
{
  joystickLoop();

  // VRx = analogRead(VRxPin);
  // VRy = analogRead(VRyPin);
  SW = buttonReadState();

  stateJoystick = joystickAxisReadState();

  // print the results to the Serial Monitor:
  // Serial.print("VRrx = ");
  // Serial.print(VRx);
  // Serial.print("\tVRry = ");
  // Serial.print(VRy);
  // Serial.print("\tSW = ");
  // Serial.println(SW);
  Serial.print("\tJoystick = ");
  Serial.println(stateJoystick);

  delay(200);
}
