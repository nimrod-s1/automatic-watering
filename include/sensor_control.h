#pragma once
#include <Arduino.h>

extern const int DRY_VALUE;
extern const int WET_VALUE;

void sensorInit(int pin_soil);

int readAveragedADC(int pin_soil, int samples = 100);
int getMoisturePercent(int pin_soil, int dry = DRY_VALUE, int wet = WET_VALUE);