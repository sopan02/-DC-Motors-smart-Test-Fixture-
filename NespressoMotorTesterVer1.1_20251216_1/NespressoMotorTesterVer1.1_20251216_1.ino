#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h> 
#include "Config.h"
#include "Utils.h"
#include "Test24VDCMotor.h"
#include "Test24VLiftingMotor.h"
#include "Test120VDCMotor.h"

LiquidCrystal_I2C lcd(0x27, 20, 4); 

void setup() 
{
  Serial.begin(115200);
  setupPins();
  setupLCD();
  setupInterrupts();
  wdt_enable(WDTO_8S); 
  Serial.println("System Initialized");
}
void loop() 
{
  wdt_reset(); 
  waitForFixtureConnection();
  waitForDUTPlacement();
  waitForDoorClosed();
  waitForStartButton();

  int dutId = readDUTID();
  String dutName = getDUTName(dutId);
  displayLCD("TESTING " + dutName);
  Serial.print("Testing DUT ID: "); Serial.println(dutId);
  Serial.println("DUT Name: " + dutName);

  bool testResult = false;
  if (dutId == 2) testResult = test24VDCMotor();
  else if (dutId == 5) testResult = test24VLiftingMotor();
  else if (dutId == 7) testResult = test120VDCMotor();
  else 
  {
    handleError("UNKNOWN DUT ID: " + String(dutId));
    return;
  }
  if (testResult) 
  {
    displayLCD("TEST PASSED \nREMOVE DUT");
    digitalWrite(passLamp, LOW);
    digitalWrite(testLamp, HIGH); 
  } 
  else 
  {
    displayLCD("TEST FAILED \nREMOVE DUT");
    digitalWrite(failLamp, LOW); 
    digitalWrite(testLamp, HIGH); 
  }
  waitForDUTRemoval();
  digitalWrite(passLamp, HIGH); 
  digitalWrite(failLamp, HIGH); 
  resetSystem();
}