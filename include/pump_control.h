#pragma once
#include <Arduino.h>

// קבועי חומרה
extern const int FIRST_PIN;
extern const int PWM_FREQ_HZ;    // תדר PWM ב-Hz
extern const int PWM_RES_BITS;   // רזולוציה (בביטים)

// פונקציות שליטה במשאבה
void pumpInit(int pin_pump);                     // אתחול
void pumpOn(int pin_pump);                       // הדלקה מלאה
void pumpOff(int pin_pump);                      // כיבוי
void pumpToggle(int pin_pump);
bool pumpIsOn(int pin_pump);
void setPumpPercent(int pin_pump, uint8_t pct);  // שליטה בעוצמה (0–100%)
void pumpSetActiveLow(bool en);    // אם המודול שלך אקטיבי-LOW
