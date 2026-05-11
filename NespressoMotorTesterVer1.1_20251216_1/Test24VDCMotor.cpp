#include <Arduino.h>
#include <avr/wdt.h>
#include "Test24VDCMotor.h"

bool test24VDCMotor() 
{
  bool overallPass = true;
  digitalWrite(testLamp, LOW);
  digitalWrite(haltRelay, HIGH);
  digitalWrite(switchRelay1, LOW);
  digitalWrite(switchRelay2, LOW);

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
 
  // === FULL STALL TEST (exactly same as original) ===
  if (digitalRead(rpmPin) != HIGH)
  {
    displayLCD("CHECKING RPM SIGNAL");
    Serial.println("RPM Pin not HIGH, starting rotate-check cycle at PWM 10");
    bool positionConfirmed = false;
    const int rotationDuration = 5;
    const int debounceDelay = 10;
    unsigned long rpmStart = millis();
    const unsigned long rpmTimeout = 30000;
    int rotationAttempts = 0;
    int highDetectionCount = 0;
    while (!positionConfirmed && (millis() - rpmStart < rpmTimeout)) 
    {
      wdt_reset();
      rotationAttempts++;
       Serial.print("Rotation attempt "); Serial.print(rotationAttempts); Serial.println(": rotating at PWM 10 for 10ms");
      analogWrite(pwmPin, 10);
      delay(rotationDuration);
      analogWrite(pwmPin, 0);
      if (digitalRead(rpmPin) == HIGH) 
      {
        delay(debounceDelay);
        if (digitalRead(rpmPin) == HIGH) 
        {
          highDetectionCount++;
          Serial.print("RPM Pin HIGH detected (count: "); Serial.print(highDetectionCount); Serial.println("), verifying position...");
          bool stableHigh = true;
          for (int i = 0; i < 5; i++) 
          {
            delay(5);
            bool currentState = digitalRead(rpmPin);
            Serial.print("Check "); Serial.print(i + 1); Serial.print(": rpmPin = "); Serial.println(currentState ? "HIGH" : "LOW");
            if (!currentState) 
            {
              stableHigh = false;
              Serial.println("RPM Pin not stable, resuming rotate-check cycle");
              break;
            }
          }
          if (stableHigh) 
          {
            positionConfirmed = true;
            Serial.print("RPM Pin stable HIGH confirmed (after "); Serial.print(highDetectionCount); Serial.println(" HIGH detections), proceeding with test");
          }
        }
      }
      if (!checkFixtureConnection()) 
      {
        handleError("DUT Fixture Disconnected");
        analogWrite(pwmPin, 0);
        return false;
      }
      if (!checkDUTPlacement()) 
      {
        handleError("DUT Removed");
        analogWrite(pwmPin, 0);
        return false;
      }
      if (!checkDoorClosed()) 
      {
        handleError("Door Opened");
        analogWrite(pwmPin, 0);
        return false;
      }
      if (!checkStopButton()) 
      {
        handleError("Test Aborted - Stop Button Pressed");
        analogWrite(pwmPin, 0);
        return false;
      }
      if (abortTest) 
      {
        handleError("Test Aborted by Interrupt");
        analogWrite(pwmPin, 0);
        return false;
      }
    }
    if (!positionConfirmed) 
    {
      Serial.print("Total rotation attempts: "); Serial.print(rotationAttempts); 
      Serial.print(", Total HIGH detections: "); Serial.println(highDetectionCount);
      handleError("Timeout: Failed to detect stable RPM Pin HIGH");
      analogWrite(pwmPin, 0);
      return false;
    }
    analogWrite(pwmPin, 0);
    Serial.println("RPM Pin HIGH detected, proceeding with test");
  }
  displayLCD("ENGAGING ACTUATOR");
  Serial.println("ENGAGING ACTUATOR WAIT 7 SECONDS");
  digitalWrite(relayActuator1, HIGH);
  delay(100);
  digitalWrite(relayActuator2, HIGH);
 
  unsigned long startTime = millis();
  float maxI = 0.0;
  int count = 0;
  while (millis() - startTime < 7000) 
  {
    wdt_reset();
    if (!checkFixtureConnection()) 
    {
      handleError("DUT Fixture Disconnected");
      digitalWrite(relayActuator1, LOW);
      digitalWrite(relayActuator2, LOW);
      analogWrite(pwmPin, 0);
      return false;
    }
    if (!checkDUTPlacement()) 
    {
      handleError("DUT Removed");
      digitalWrite(relayActuator1, LOW);
      digitalWrite(relayActuator2, LOW);
      analogWrite(pwmPin, 0);
      return false;
    }
    if (!checkDoorClosed()) 
    {
      handleError("Door Opened");
      digitalWrite(relayActuator1, LOW);
      digitalWrite(relayActuator2, LOW);
      analogWrite(pwmPin, 0);
      attachInterrupt(digitalPinToInterrupt(rpmPin), rpmISR, RISING);
      return false;
    }
    if (!checkStopButton()) 
    {
      handleError("Test Aborted - Stop Button Pressed");
      analogWrite(pwmPin, 0);
      attachInterrupt(digitalPinToInterrupt(rpmPin), rpmISR, RISING);
      return false;
    }

    float I = readCurrent(true);
    if (I > maxI) maxI = I;
    count++;

    Serial.print("Stall Current: ");
    Serial.print(I, 2);
    Serial.println(" A");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("STALL TEST RUNNING");
    lcd.setCursor(0, 1);
    lcd.print("I:"); lcd.print(I, 1); lcd.print("A");

    wdt_reset();
    delay(100);
  }
  analogWrite(pwmPin, 0);
  attachInterrupt(digitalPinToInterrupt(rpmPin), rpmISR, RISING);

  if (count == 0) 
  {
    Serial.println("ERROR: No stall current samples collected");
    overallPass = false;
    return overallPass;
  }

  Serial.print("Peak Stall Current = ");
  Serial.print(maxI, 2);
  Serial.println(" A");

  if (maxI < 2.0) 
  {
    displayLCD("RELEASING ACTUATOR");
    Serial.println("RELEASING ACTUATOR WAIT 7 SECONDS");
    digitalWrite(relayActuator1, LOW);
    delay(100);
    digitalWrite(relayActuator2, LOW);
    startTime = millis();
    while (millis() - startTime < 7000) 
    {
      wdt_reset();
      if (!checkFixtureConnection()) 
      {
        handleError("DUT Fixture Disconnected");
        analogWrite(pwmPin, 0);
        return false;
      }
      if (!checkDUTPlacement()) 
      {
        handleError("DUT Removed");
        analogWrite(pwmPin, 0);
        return false;
      }
      if (!checkDoorClosed()) 
      {
        handleError("Door Opened");
        analogWrite(pwmPin, 0);
        return false;
      }
      if (!checkStopButton()) 
      {
        handleError("Test Aborted - Stop Button Pressed");
        analogWrite(pwmPin, 0);
        return false;
      }
      if (abortTest) 
      {
        handleError("Test Aborted by Interrupt");
        analogWrite(pwmPin, 0);
        return false;
      }
      delay(100);
    }
    Serial.println("Actuator release complete");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("STALL TEST FAILED");
    lcd.setCursor(0, 1);
    lcd.print("PEAK I: ");
    lcd.print(maxI, 1);
    lcd.print("A < 2A");
    delay(1000);
    overallPass = false;
    Serial.println("FAIL: Peak Stall Current < 2A");
    analogWrite(pwmPin, 0);
    return overallPass;
  } 
  else 
  {
    displayLCD("RELEASING ACTUATOR");
    Serial.println("RELEASING ACTUATOR WAIT 7 SECONDS");
    digitalWrite(relayActuator1, LOW);
    delay(100);
    digitalWrite(relayActuator2, LOW);
    startTime = millis();
    while (millis() - startTime < 7000) 
    {
      wdt_reset();
      if (!checkFixtureConnection()) 
      {
        handleError("DUT Fixture Disconnected");
        analogWrite(pwmPin, 0);
        return false;
      }
      if (!checkDUTPlacement()) 
      {
        handleError("DUT Removed");
        analogWrite(pwmPin, 0);
        return false;
      }
      if (!checkDoorClosed()) 
      {
        handleError("Door Opened");
        analogWrite(pwmPin, 0);
        return false;
      }
      if (!checkStopButton()) 
      {
        handleError("Test Aborted - Stop Button Pressed");
        analogWrite(pwmPin, 0);
        return false;
      }
      if (abortTest) 
      {
        handleError("Test Aborted by Interrupt");
        analogWrite(pwmPin, 0);
        return false;
      }
      delay(100);
    }
    Serial.println("Actuator release complete");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("STALL TEST PASSED");
    lcd.setCursor(0, 1);
    lcd.print("PEAK I: ");
    lcd.print(maxI, 1);
    lcd.print("A");
    delay(1000);
    Serial.println("PASS: Peak Stall Current OK");
  }
  analogWrite(pwmPin, 0);
  
  Serial.println("Starting PWM Tests");
  if (!singlePWMTest("25% PWM", 64, 6.0, 1.0, 4000)) 
  {
    overallPass = false;
    Serial.println("25% PWM test failed");
    return overallPass;
  }
  if (!singlePWMTest("50% PWM", 128, 12.0, 2.0, 7000)) 
  {
    overallPass = false;
    Serial.println("50% PWM test failed");
    return overallPass;
  }
  Serial.println("All PWM tests completed");
  return overallPass;
}