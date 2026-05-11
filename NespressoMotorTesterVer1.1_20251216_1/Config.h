#include "pins_arduino.h"
#ifndef CONFIG_H
#define CONFIG_H
#include <LiquidCrystal_I2C.h>
extern LiquidCrystal_I2C lcd;
const int idPins[3] = {16, 15, 14};
const int irPlacement = 6;
const int rpmPin = 18;
const int doorPin = 7;
const int voltPin = A0;
const int currPin = A1;
const int pwmPin = 10;
const int relaySpare1 = 8;
const int relaySpare2 = 5;
const int haltRelay = 11;
const int switchRelay1 = 22;
const int switchRelay2 = 23;
const int relayActuator1 = 26;
const int relayActuator2 = 27;
const int testLamp = 28;
const int passLamp = 29;
const int failLamp = 30;
const int buzzer = 31;
const int startButton = 33;
const int stopButton = 32;
extern volatile int rpmCount;
extern unsigned long lastRPMCheck;
extern volatile bool abortTest;
struct DUTConfig
{
float Vref;
float voltScale;
float sensitivity;
float zeroCurrent;
};
// DUT-specific configurations 
const DUTConfig dutConfigs[3] =
{
// DUT ID 2: 24V DC Motor
{
5.0,              // Vref
24.0 / 1023.0,    // voltScale
0.05,             // sensitivity
2.53              // zeroCurrent
},
// DUT ID 5: 24V Lifting Motor
{
5.0,              // Vref
24.0 / 1023.0,    // voltScale
0.05,             // sensitivity
2.53              // zeroCurrent
},
// DUT ID 7: 120V DC Motor
{
5.0,              // Vref
110.0 / 5.0,      // voltScale
0.05,             // sensitivity
2.53              // zeroCurrent
}
};
// Function to get DUT configuration based on ID
inline DUTConfig getDUTConfig(int dutId)
{
if (dutId == 2) return dutConfigs[0]; // 24V DC Motor
if (dutId == 5) return dutConfigs[1]; // 24V Lifting Motor
if (dutId == 7) return dutConfigs[2]; // 120V DC Motor
return dutConfigs[0]; // Default to 24V DC Motor config if ID is invalid
}
struct PWMTestTolerances
{
float vTolerance;
float iTolerance;
float rpmTolerance;
};
// PWM-specific tolerances for each DUT [dutIndex][pwmTestIndex]
// dutIndex: 0=24V DC Motor (id2), 1=24V Lifting Motor (id5), 2=120V DC Motor (id7)
// pwmTestIndex: 0=25%, 1=50%, 2=75%, 3=100% (mapped by pwmValue per DUT)
const PWMTestTolerances pwmTolerances[3][4] =
{
// DUT 0: 24V DC Motor (id 2) 
{
{5.0f, 0.5f, 1500.0f},  // 25% PWM 
{5.0f, 1.0f, 1500.0f},  // 50% PWM
{5.0f, 1.0f, 1500.0f},  // 75% PWM
{5.0f, 1.5f, 1500.0f}   // 100% PWM
},
// DUT 1: 24V Lifting Motor (id 5) 
{
{6.0f, 0.3f, 70.0f},    // 25%
{5.0f, 0.3f, 90.0f},    // 50%
{5.0f, 0.3f, 60.0f},    // 75%
{5.0f, 0.3f, 70.0f}     // 100%
},
// DUT 2: 120V DC Motor (id 7) 
{
{0.0f, 0.3f, 3000.0f}, // 25% PWM
{0.0f, 0.4f, 2000.0f}, // 50% PWM
{0.0f, 0.4f, 2000.0f}, // 75% PWM
{0.0f, 0.4f, 2000.0f}  // 100% PWM
}
};
#endif