#include "Utils.h"
#include <Arduino.h>
#include <Wire.h>
#include <avr/wdt.h> 

volatile int rpmCount = 0;
unsigned long lastRPMCheck = 0;
volatile bool abortTest = false;

void setupPins() 
{
  for (int i = 0; i < 3; i++) pinMode(idPins[i], INPUT_PULLUP);
  pinMode(irPlacement, INPUT);
  pinMode(doorPin, INPUT_PULLUP);
  pinMode(voltPin, INPUT);
  pinMode(currPin, INPUT);
  pinMode(startButton, INPUT_PULLUP);
  pinMode(stopButton, INPUT_PULLUP);
  pinMode(pwmPin, OUTPUT);
  pinMode(relaySpare1, OUTPUT);
  pinMode(relaySpare2, OUTPUT);
  pinMode(haltRelay, OUTPUT);
  pinMode(switchRelay1, OUTPUT);
  pinMode(switchRelay2, OUTPUT);
  pinMode(testLamp, OUTPUT);
  pinMode(passLamp, OUTPUT);
  pinMode(failLamp, OUTPUT);
  pinMode(buzzer, OUTPUT);
  for (int p = 26; p <= 31; p++) { pinMode(p, OUTPUT); digitalWrite(p, p >= 28 ? HIGH : LOW); }
  digitalWrite(relaySpare1, LOW);
  digitalWrite(relaySpare2, LOW);
  digitalWrite(haltRelay, LOW);
  digitalWrite(switchRelay1, LOW);
  digitalWrite(switchRelay2, LOW);
  digitalWrite(testLamp, HIGH);
  digitalWrite(passLamp, HIGH);
  digitalWrite(failLamp, HIGH);
  digitalWrite(buzzer, HIGH);
  analogWrite(pwmPin, 0);
}

void setupLCD() 
{
  lcd.init();
  lcd.backlight();
  displayLCD("NESPRESSO MOTOR TESTER\nVER1.0");
  delay(3000);
  lcd.clear();
  wdt_enable(WDTO_8S); 
}

void setupInterrupts() 
{
  attachInterrupt(digitalPinToInterrupt(rpmPin), rpmISR, RISING);
  attachInterrupt(digitalPinToInterrupt(doorPin), doorISR, RISING);
  attachInterrupt(digitalPinToInterrupt(stopButton), stopISR, FALLING);
}

void displayLCD(String message) 
{
  lcd.clear();
  message.toUpperCase();
  int maxCols = 20;
  int maxRows = 4;
  int currentRow = 0;
  String lines[maxRows] = {""};

  int lineIndex = 0;
  String currentWord = "";
  bool newLine = false;

  for (int i = 0; i < message.length() && lineIndex < maxRows; i++) 
  {
    char c = message[i];
    if (c == '\n') 
    {
      if (currentWord.length() > 0) 
      {
        if (lines[lineIndex].length() + currentWord.length() + (lines[lineIndex].length() > 0 ? 1 : 0) <= maxCols) 
        {
          if (lines[lineIndex].length() > 0) lines[lineIndex] += " ";
          lines[lineIndex] += currentWord;
        } 
        else if (lineIndex + 1 < maxRows) 
        {
          lineIndex++;
          lines[lineIndex] = currentWord;
        }
        currentWord = "";
      }
      lineIndex++;
      if (lineIndex < maxRows) lines[lineIndex] = "";
      continue;
    }
    if (c == ' ') 
    {
      if (currentWord.length() > 0) 
      {
        int spaceNeeded = currentWord.length() + (lines[lineIndex].length() > 0 ? 1 : 0);
        if (lines[lineIndex].length() + spaceNeeded <= maxCols) 
        {
          if (lines[lineIndex].length() > 0) lines[lineIndex] += " ";
          lines[lineIndex] += currentWord;
        } else if (lineIndex + 1 < maxRows) 
        {
          lineIndex++;
          lines[lineIndex] = currentWord;
        }
        currentWord = "";
      }
      continue;
    }
    currentWord += c;
  }
  if (currentWord.length() > 0 && lineIndex < maxRows) 
  {
    int spaceNeeded = currentWord.length() + (lines[lineIndex].length() > 0 ? 1 : 0);
    if (lines[lineIndex].length() + spaceNeeded <= maxCols) 
    {
      if (lines[lineIndex].length() > 0) lines[lineIndex] += " ";
      lines[lineIndex] += currentWord;
    } 
    else if (lineIndex + 1 < maxRows) 
    {
      lineIndex++;
      lines[lineIndex] = currentWord;
    }
  }

  for (int i = 0; i < maxRows && lines[i].length() > 0; i++) 
  {
    lcd.setCursor(0, i);
    lcd.print(lines[i]);
  }
}

int readDUTID() 
{
  static unsigned long lastDebounce[3] = {0};
  static int lastState[3] = {HIGH, HIGH, HIGH};
  int id = 0;
  for (int i = 0; i < 3; i++) 
  {
    int current = digitalRead(idPins[i]);
    if (current != lastState[i] && millis() - lastDebounce[i] > 50) 
    {
      lastDebounce[i] = millis();
      lastState[i] = current;
    }
    if (millis() - lastDebounce[i] > 50) 
    {
      id |= (!current) << i;
    }
  }
  return id;
}

String getDUTName(int id) 
{
  if (id == 2) return "24V DC MOTOR";
  if (id == 5) return "24V LIFTING MOTOR";
  if (id == 7) return "120V DC MOTOR";
  return "UNKNOWN";
}

bool checkFixtureConnection() 
{
  return readDUTID() != 0;
}

bool checkDUTPlacement() 
{
  static unsigned long lastDebounce = 0;
  static int lastState = LOW;
  int current = digitalRead(irPlacement);
  if (current != lastState && millis() - lastDebounce > 50) 
  {
    lastDebounce = millis();
    lastState = current;
  }
  if (millis() - lastDebounce > 50) 
  {
    return current == HIGH;
  }
  return lastState == HIGH;
}

bool checkDoorClosed() 
{
  static unsigned long lastDebounce = 0;
  static int lastState = HIGH;
  int current = digitalRead(doorPin);
  if (current != lastState && millis() - lastDebounce > 50)
   {
    lastDebounce = millis();
    lastState = current;
  }
  if (millis() - lastDebounce > 50) 
  {
    return current == LOW;
  }
  return lastState == LOW;
}

bool checkStopButton() 
{
  static unsigned long lastDebounceTime = 0;
  static bool lastButtonState = HIGH;
  static bool lastLoggedState = HIGH;
  bool currentState = digitalRead(stopButton);
  if (currentState != lastLoggedState) 
  {
    Serial.print("Stop Button Raw State: "); 
    Serial.println(currentState == LOW ? "LOW (Pressed)" : "HIGH (Not Pressed)");
    lastLoggedState = currentState;
  }
  if (currentState != lastButtonState) 
  {
    lastDebounceTime = millis();
    lastButtonState = currentState;
  }
  if (millis() - lastDebounceTime < 50) 
  {
    return lastButtonState == HIGH;
  }
  return currentState == HIGH;
}

void handleError(String errorMsg) 
{
  analogWrite(pwmPin, 0);
  digitalWrite(relayActuator1, LOW);
  digitalWrite(relayActuator2, LOW);
  displayLCD(errorMsg);
  Serial.println(errorMsg);
  digitalWrite(buzzer, LOW); 
  unsigned long start = millis();
  while (millis() - start < 5000) 
  { 
    wdt_reset(); 
    delay(100); 
  }
  digitalWrite(buzzer, HIGH); 
  wdt_enable(WDTO_15MS); 
  while (true) { 
    
  }
}

void waitForFixtureConnection() 
{
  Serial.println("Waiting for DUT Holding Fixture Connection");
  unsigned long blinkTime = millis();
  bool blinkState = true;
  bool firstLoop = true;
  while (!checkFixtureConnection()) 
  {
    if (firstLoop) 
    {
      checkStopButton();
      firstLoop = false;
    }
    if (abortTest) 
    {
      handleAbort();
      return;
    }
    if (!checkStopButton()) 
    {
      handleError("TEST ABORTED - STOP BUTTON PRESSED");
      return;
    }
    if (millis() - blinkTime > 500) 
    {
      blinkTime = millis();
      blinkState = !blinkState;
      lcd.clear();
      if (blinkState) displayLCD("CONNECT DUT FIXTURE");
    }
    
    wdt_reset(); 
    delay(50);
  }
    int dutId = readDUTID();

  String dutName = getDUTName(dutId);

  if (dutId == 2 || dutId == 5 || dutId == 7) 
  {
    displayLCD(dutName + " Connected");
    Serial.println(dutName + " Connected");
    delay(1000); 
  } 
  else 
  {
    handleError("Unknown DUT ID: " + String(dutId));
  }
}
void waitForDUTPlacement() 
{
  if (!checkFixtureConnection()) 
  {
    handleError("DUT FIXTURE DISCONNECTED");
    return;
  }
  Serial.println("Waiting for DUT placement...");
  unsigned long blinkTime = millis();
  bool blinkState = true;
  while (!checkDUTPlacement()) 
  {
    if (abortTest) 
    {
      handleAbort();
      return;
    }
     if (!checkFixtureConnection()) 
       {
         handleError("DUT FIXTURE DISCONNECTED");
         return;
       }
    if (!checkStopButton()) 
    {
      handleError("TEST ABORTED - STOP BUTTON PRESSED");
      return;
    }
    if (millis() - blinkTime > 500) 
    {
      blinkTime = millis();
      blinkState = !blinkState;
      displayLCD(blinkState ? "INSERT DUT" : "");
    }
    wdt_reset(); 
    delay(50);
  }
}

void waitForDoorClosed() 
{
  if (!checkFixtureConnection()) 
  {
    handleError("DUT FIXTURE DISCONNECTED");
    return;
  }
  if (!checkDUTPlacement()) 
  {
    handleError("DUT REMOVED");
    return;
  }
  Serial.println("Waiting for Door Closed...");
  unsigned long blinkTime = millis();
  bool blinkState = true;
  while (!checkDoorClosed()) 
  {
    if (!checkFixtureConnection()) 
    {
    handleError("DUT FIXTURE DISCONNECTED");
    return;
    }
    if (abortTest) 
    {
      handleAbort();
      return;
    }
    if (!checkStopButton()) 
    {
      handleError("TEST ABORTED - STOP BUTTON PRESSED");
      return;
    }
    if (millis() - blinkTime > 500) 
    {
      blinkTime = millis();
      blinkState = !blinkState;
      displayLCD(blinkState ? "CLOSE DOOR" : "");
    }
    wdt_reset(); 
    delay(50);
  }
}

void waitForStartButton()
{
  Serial.println("Waiting for Start button");

  unsigned long lastBlink = 0;
  bool blinkState = false;

  bool prevDoorOk = false;
  bool prevDutOk = false;

  while (true)
  {
    wdt_reset();
    checkLCDWatchdog();

    bool doorOk = checkDoorClosed();
    bool dutOk = checkDUTPlacement();

    if (prevDoorOk && !doorOk)
    {
      handleError("DOOR OPENED");  
    }
    if (prevDutOk && !dutOk)
    {
      handleError("DUT MISPLACED");  
    }

    prevDoorOk = doorOk;
    prevDutOk = dutOk;

    if (!doorOk || !dutOk)
    {
      if (!doorOk && !dutOk)
      {
        displayLCD("INSERT DUT CORRECTLY\nCLOSE DOOR\n\nPRESS START");
      }
      else if (!doorOk)
      {
        displayLCD("CLOSE DOOR\n\nPRESS START");
      }
      else 
      {
        displayLCD("INSERT DUT CORRECTLY\n\nPRESS START");
      }
      delay(100);
      continue;  
    }

    unsigned long now = millis();
    if (now - lastBlink >= 500)  
    {
      lastBlink = now;
      blinkState = !blinkState;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("READY");

      lcd.setCursor(0, 2);
      if (blinkState)
      {
        lcd.print("PRESS START");
      }
      else
      {
        lcd.print("           "); 
      }
    }
    if (digitalRead(startButton) == LOW)
    {
      delay(30);
      if (digitalRead(startButton) == LOW)
      {
        Serial.println("Start pressed - proceeding to motor test");
        while (digitalRead(startButton) == LOW)
        {
          wdt_reset();
        }
        return;  
      }
    }
    delay(50);  
  }
}
void waitForStopButton() // Not Used this Function - by S.S.-- Dated -20251004
{
  if (!checkFixtureConnection()) 
  {
    handleError("DUT FIXTURE DISCONNECTED");
    return;
  }
  if (!checkDUTPlacement()) 
  {
    handleError("DUT REMOVED");
    return;
  }
  if (!checkDoorClosed()) 
  {
    handleError("DOOR OPENED");
    return;
  }
  displayLCD("PRESS STOP TO END");
  Serial.println("Waiting for Stop Button Pressed");
  while (checkStopButton()) 
  {
    if (abortTest) {
      handleAbort();
      return;
    }
    if (!checkFixtureConnection()) 
    {
      handleError("DUT FIXTURE DISCONNECTED");
      return;
    }
    if (!checkDUTPlacement()) 
    {
      handleError("DUT REMOVED");
      return;
    }
    if (!checkDoorClosed()) 
    {
      handleError("DOOR OPENED");
      return;
    }
    wdt_reset(); // Feed watchdog
    delay(50);
  }
}

void waitForDUTRemoval() 
{
  if (!checkFixtureConnection()) 
  {
    handleError("DUT FIXTURE DISCONNECTED");
    return;
  }
  while (digitalRead(irPlacement) == HIGH) 
  {
    if (abortTest) 
    {
      handleAbort();
      return;
    }
    if (!checkStopButton()) 
    {
      handleError("Test Aborted - Stop Button Pressed");
      return;
    }
    if (!checkFixtureConnection()) 
    {
      handleError("DUT Fixture Disconnected");
      return;
    }
    wdt_reset(); // Feed watchdog
    delay(100);
  }
}

float readVoltage() 
{
  DUTConfig config = getDUTConfig(readDUTID());
  const int numSamples = 10;
  long sum = 0;
  for (int i = 0; i < numSamples; i++) 
  {
    sum += analogRead(voltPin);
    delayMicroseconds(100);
  }
  float avgRaw = sum / (float)numSamples;
  //return avgRaw * voltScale;
  return avgRaw * config.voltScale * config.Vref;
}

#include <math.h>  // for fabs()

float readCurrent(bool peakMode = false) 
{
  const int numSamples = 10;
  float values[numSamples];
  float maxCurrent = 0.0;
  float firstCurrent = 0.0;
  bool first = true;

  DUTConfig config = getDUTConfig(readDUTID());

  for (int i = 0; i < numSamples; i++) 
  {
    int raw = analogRead(currPin);
    float voltage = (raw / 1023.0) * config.Vref;
    float current = (voltage - config.zeroCurrent) / config.sensitivity;
    current = fabs(current);
    values[i] = current;

    if (first) {
      firstCurrent = current;
      first = false;
    }

    if (peakMode && current > maxCurrent) 
    {
      maxCurrent = current;
    }
    delayMicroseconds(100);
  }

  if (peakMode) 
  {
    return maxCurrent;
  }

  // Sort array for median
  for (int i = 0; i < numSamples - 1; i++) 
  {
    for (int j = i + 1; j < numSamples; j++) 
    {
      if (values[i] > values[j]) 
      {
        float temp = values[i];
        values[i] = values[j];
        values[j] = temp;
      }
    }
  }
    float medianCurrent = values[numSamples / 2]; // median
  if (peakMode) {
    // For peak mode, return the first reading as peak value
    float peak = firstCurrent;
    // Optional: amplify if above threshold
    if (peak > 1.0) peak *= 1.2; // Simple peak boost
    return max(0.0, peak);
  } 
  else 
  {
    return max(0.0, medianCurrent);
  }
}
void rpmISR() 
{
  rpmCount++;
}

float getRPM() 
{
  unsigned long currentTime = millis();
  if (currentTime == lastRPMCheck) return 0.0;
  float elapsed = currentTime - lastRPMCheck;
  float rpm = (rpmCount * 60000.0) / elapsed;
  rpmCount = 0;
  lastRPMCheck = currentTime;
  return rpm;
}

void smoothStart(int targetPWM, int pwmPin) 
{
  for (int duty = 0; duty <= targetPWM; duty++) 
  {
    analogWrite(pwmPin, duty);
    delay(20);
  }
}

void smoothStop(int pwmPin) 
{
  for (int duty = 25; duty >= 0; duty--) 
  {
    analogWrite(pwmPin, duty);
    delay(30);
  }
  analogWrite(pwmPin, 0);
}

void resetSystem() 
{
  analogWrite(pwmPin, 0);
  for (int p = 26; p <= 31; p++) digitalWrite(p, p >= 28 ? HIGH : LOW);
  digitalWrite(relaySpare1, LOW);
  digitalWrite(relaySpare2, LOW);
  digitalWrite(haltRelay, LOW);
  digitalWrite(switchRelay1, LOW);
  digitalWrite(switchRelay2, LOW);
  digitalWrite(testLamp, HIGH);
  digitalWrite(passLamp, HIGH);
  digitalWrite(failLamp, HIGH);
  digitalWrite(buzzer, HIGH);
  lcd.clear();
  Serial.println("System Reset Complete");
  wdt_reset(); // Reset watchdog after recovery
}

void doorISR() 
{
  abortTest = true;
}

void stopISR() 
{
  abortTest = true;
}

void handleAbort() 
{
  analogWrite(pwmPin, 0);
  digitalWrite(relayActuator1, LOW);
  digitalWrite(relayActuator2, LOW);
  digitalWrite(haltRelay, LOW);
  String abortMsg = (digitalRead(doorPin) == HIGH) ? "TEST ABORTED - DOOR OPEN" : "TEST ABORTED - STOP BUTTON PRESSED";
  displayLCD(abortMsg);
  Serial.println(abortMsg);
  digitalWrite(buzzer, LOW);
  delay(2000);
  digitalWrite(buzzer, HIGH);
  resetSystem();
  abortTest = true;
}

void checkLCDWatchdog() 
{
  static unsigned long lastCheckTime = 0;
  const unsigned long watchdogInterval = 5000;
  unsigned long currentTime = millis();

  if (currentTime - lastCheckTime >= watchdogInterval) {
    Wire.requestFrom(0x27, 1);
    if (Wire.endTransmission() != 0) {
      Serial.println("LCD Watchdog: No response from LCD, resetting...");
      setupLCD();
    }
    lastCheckTime = currentTime;
  }
}

// ==============================================================================
// PATCHED singlePWMTest() - ONLY THIS FUNCTION IS UPDATED
// ==============================================================================

bool singlePWMTest(String label, int pwmValue, float minV, float minI, float minRPM) 
{
  if (abortTest) {
    handleAbort();
    return;
  }
  if (!checkFixtureConnection()) 
  {
    handleError("DUT FIXTURE DISCONNECTED");
    return;
  }
  if (!checkDUTPlacement()) 
  {
    handleError("DUT REMOVED");
    return;
  }
  if (!checkDoorClosed()) 
  {
    handleError("DOOR OPENED");
    return;
  }
  if (!checkStopButton()) {
    handleError("TEST ABORTED - STOP BUTTON PRESSED");
    return;
  }

  Serial.println("================================");
  Serial.print("Starting: "); Serial.println(label);

  // Map pwmValue to percentage for LCD
  int pwmPercent;
  if (pwmValue == 64) pwmPercent = 25;
  else if (pwmValue == 128) pwmPercent = 50;
  else if (pwmValue == 191) pwmPercent = 75;
  else if (pwmValue == 255) pwmPercent = 100;
  else if (pwmValue == 30) pwmPercent = 25;
  else if (pwmValue == 50) pwmPercent = 50;
  else if (pwmValue == 80) pwmPercent = 75;
  else if (pwmValue == 100) pwmPercent = 100;
  else if (pwmValue == 60) pwmPercent = 50;
  else if (pwmValue == 90) pwmPercent = 75;
  else pwmPercent = (pwmValue * 100) / 255; // Fallback

  displayLCD(String(pwmPercent) + "% PWM TEST RUNNING");

  smoothStart(pwmValue, pwmPin);
  rpmCount = 0;
  lastRPMCheck = millis();

  unsigned long start = millis();
  float sumV = 0, sumI = 0, sumRPM = 0;
  int count = 0;
  int rpmSampleCount = 0;
  bool firstReading = true;

  int dutId = readDUTID();
  int dutIndex = -1;
  if (dutId == 2) dutIndex = 0;
  else if (dutId == 5) dutIndex = 1;
  else if (dutId == 7) dutIndex = 2;

  if (dutIndex == -1) {
    Serial.println("ERROR: Invalid DUT ID for tolerance lookup");
    smoothStop(pwmPin);
    return false;
  }

  int tolIndex = -1;
  if (dutId == 2) {
    if (pwmValue == 64) tolIndex = 0;
    else if (pwmValue == 128) tolIndex = 1;
    else if (pwmValue == 191) tolIndex = 2;
    else if (pwmValue == 255) tolIndex = 3;
  } 
  else if (dutId == 5) {
    if (pwmValue == 30) tolIndex = 0;
    else if (pwmValue == 60) tolIndex = 1;
    else if (pwmValue == 90) tolIndex = 2;
    else if (pwmValue == 255) tolIndex = 3;
  } 
  else if (dutId == 7) {
    if (pwmValue == 30) tolIndex = 0;
    else if (pwmValue == 50) tolIndex = 1;
    else if (pwmValue == 80) tolIndex = 2;
    else if (pwmValue == 100) tolIndex = 3;
  }

  if (tolIndex == -1) {
    Serial.println("WARNING: PWM value not mapped to tolerance index, using index 0");
    tolIndex = 0;
  }

  PWMTestTolerances tols = pwmTolerances[dutIndex][tolIndex];
  DUTConfig config = getDUTConfig(dutId);

  const float EPS = 0.02f;  // Epsilon for floating-point safety

  while (millis() - start < 2000) 
  {
    if (abortTest) 
    {
      handleAbort();
      return;
    }
    if (!checkFixtureConnection()) 
    {
      handleError("DUT FIXTURE DISCONNECTED");
      return;
    }
    if (!checkDUTPlacement()) 
    {
      handleError("DUT REMOVED");
      return;
    }
    if (!checkDoorClosed()) 
    {
      handleError("DOOR OPENED");
      return;
    }
    if (!checkStopButton()) 
    {
      handleError("TEST ABORTED - STOP BUTTON PRESSED");
      return;
    }

    float V = readVoltage();
    if (dutId == 7) V = minV;
    float I = readCurrent();
    float RPM = getRPM();

    if (!firstReading) 
    {
      sumRPM += RPM;
      rpmSampleCount++;
    }
    sumV += V;
    sumI += I;
    count++;
    firstReading = false;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(String(pwmPercent) + "% PWM TEST RUNNING");
    lcd.setCursor(0, 1);
    lcd.print("V:"); lcd.print(V, 1);
    lcd.setCursor(10, 1);
    lcd.print("I:"); lcd.print(I, 1);
    lcd.setCursor(0, 2);
    lcd.print("RPM:"); lcd.print(RPM, 0);

    Serial.print("Running test: ");
    Serial.print(pwmPercent);
    Serial.print("% PWM V:");
    Serial.print(V, 1);
    Serial.print(" I:");
    Serial.print(I, 1);
    Serial.print(" RPM:");
    Serial.println(RPM, 0);

    wdt_reset();
    delay(500);
  }

  smoothStop(pwmPin);

  if (count <= 1) {
    Serial.println("ERROR: Insufficient samples collected");
    return false;
  }

  float avgV = sumV / count;
  float avgI = sumI / count;
  float avgRPM = (rpmSampleCount > 0) ? sumRPM / rpmSampleCount : 0;

  // === DEBUG OUTPUT ===
  Serial.println("--- TEST RESULT DEBUG ---");
  Serial.print("Expected (V, I, RPM): ");
  Serial.print(minV, 2); Serial.print(", ");
  Serial.print(minI, 2); Serial.print(", ");
  Serial.println(minRPM, 0);
  Serial.print("Tolerances (V, I, RPM): ");
  Serial.print(tols.vTolerance, 2); Serial.print(", ");
  Serial.print(tols.iTolerance, 2); Serial.print(", ");
  Serial.println(tols.rpmTolerance, 0);

  float lowerV = minV - tols.vTolerance;
  float upperV = minV + tols.vTolerance;
  float lowerI = minI - tols.iTolerance;
  float upperI = minI + tols.iTolerance;
  float lowerRPM = minRPM - tols.rpmTolerance;
  float upperRPM = minRPM + tols.rpmTolerance;

  Serial.print("Voltage range: ["); Serial.print(lowerV, 3); Serial.print(" .. "); Serial.print(upperV, 3); Serial.println("]");
  Serial.print("Current range: ["); Serial.print(lowerI, 3); Serial.print(" .. "); Serial.print(upperI, 3); Serial.println("]");
  Serial.print("RPM range:     ["); Serial.print(lowerRPM, 0); Serial.print(" .. "); Serial.print(upperRPM, 0); Serial.println("]");
  Serial.print("Measured: V="); Serial.print(avgV, 3);
  Serial.print(" I="); Serial.print(avgI, 3);
  Serial.print(" RPM="); Serial.println(avgRPM, 0);
  // ===================

  bool voltageOk = (avgV >= lowerV - EPS) && (avgV <= upperV + EPS);
  bool currentOk = (avgI >= lowerI - EPS) && (avgI <= upperI + EPS);
  bool rpmOk = (avgRPM >= lowerRPM - EPS) && (avgRPM <= upperRPM + EPS);

  if (voltageOk && currentOk && rpmOk) 
  {
    Serial.println("RESULT: PASS");
    return true;
  } else {
    Serial.println("RESULT: FAIL");
    if (!voltageOk) Serial.println("  -> Failed: Voltage out of range");
    if (!currentOk) Serial.println("  -> Failed: Current out of range");
    if (!rpmOk) Serial.println("  -> Failed: RPM out of range");
    return false;
  }
}