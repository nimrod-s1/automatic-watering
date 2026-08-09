#include "sensor_control.h"
  
const int DRY_VALUE = 3115;
const int WET_VALUE = 955;


void sensorInit(int pin_soil){
    analogReadResolution(12); // 12 bits (0-4095)
    analogSetPinAttenuation(pin_soil, ADC_11db);  // Voltage range (ADC_11db: 0-3.3V)
}

int readAveragedADC(int pin_soil, int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin_soil);
    // **תיקון קריטי 2: yield() במקום delay(2)**
    yield(); 
  }
  return (int)(sum / samples);
}


int getMoisturePercent(int pin_soil, int dry, int wet) {
  // low ADC -> wet (100%); high ADC -> dry (0%)
  int raw = readAveragedADC(pin_soil);
  long p = (long)(dry - raw) * 100 / (long)(dry - wet);
  if (p < 0) p = 0;
  if (p > 100) p = 100;
  return (int)p;
}