#ifndef UTILS_H
#define UTILS_H

#include "Config.h"

void setupPins();
void setupLCD();
void setupInterrupts();
void displayLCD(String message);
int readDUTID();
String getDUTName(int id);
void waitForFixtureConnection();
void waitForDUTPlacement();
void waitForDoorClosed();
void waitForStartButton();
void waitForStopButton();
void waitForDUTRemoval();
float readVoltage();
float readCurrent(bool peakMode = false);
void rpmISR();
float getRPM();
void smoothStart(int targetPWM, int pwmPin);
void smoothStop(int pwmPin);
void resetSystem();
void doorISR();
void stopISR();
void handleAbort();
bool checkFixtureConnection();
bool checkDUTPlacement();
bool checkDoorClosed();
bool checkStopButton();
void handleError(String errorMsg);
void checkLCDWatchdog();
bool singlePWMTest(String label, int pwmValue, float minV, float minI, float minRPM);
#endif