#include <Arduino.h>
#include "Test120VDCMotor.h"

bool test120VDCMotor() 
{
  digitalWrite(haltRelay, LOW);
  digitalWrite(switchRelay1, HIGH);
  digitalWrite(switchRelay2, HIGH);
  digitalWrite(haltRelay, LOW);
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
 
  if (!singlePWMTest("25% PWM", 30, 30.0, 0.3, 3500.0)) 
  {
    overallPass = false;
    Serial.println("25% PWM test failed");
    return overallPass;
  }
  if (!singlePWMTest("50% PWM", 50, 60.0, 0.3, 4000.0)) 
  {
    overallPass = false;
    Serial.println("50% PWM test failed");
    return overallPass;
  }
  if (!singlePWMTest("75% PWM", 80, 90.0, 0.3, 5000.0)) 
  {
    overallPass = false;
    Serial.println("75% PWM test failed");
    return overallPass;
  }
    
  return overallPass;
}