#include <Arduino.h>
#include "Test24VLiftingMotor.h"

bool test24VLiftingMotor() 
{
  digitalWrite(haltRelay, HIGH);
  digitalWrite(switchRelay1, LOW);
  digitalWrite(switchRelay2, LOW);
  digitalWrite(testLamp, LOW);
  bool overallPass = true;
  
  if (!checkFixtureConnection()) 
  {
    handleError("DUT Fixture Disconnected");
    return false;
  }
  if (!checkDUTPlacement()) 
  {
    handleError("DUT Removed");
    return false;
  }
  if (!checkDoorClosed()) 
  {
    handleError("Door Opened");
    return false;
  }
  if (!checkStopButton()) 
  {
    handleError("Test Aborted - Stop Button Pressed");
    return false;
  }

  if (!singlePWMTest("25% PWM", 30, 6.0, 0.2, 150))  
  {
    overallPass = false;
    Serial.println("25% PWM test failed");
    return overallPass;
  }
  if (!singlePWMTest("50% PWM", 60, 12.0, 0.3, 250))  
  {
    overallPass = false;
    Serial.println("50% PWM test failed");
    return overallPass;
  }
  return overallPass;
}