#include "pump_control.h"

// ===== קבועים =====
const int PWM_FREQ_HZ  = 1000;  // תדר PWM ~1kHz
const int PWM_RES_BITS = 10;    // רזולוציה 10 ביט → תחום 0..1023

const uint32_t MAX_DUTY = (1u << PWM_RES_BITS) - 1u; // ואז פחות 1 PWM_RES_BITSקובע ערך מסימלי לדיוטי;; הקטן קטן זה בעצם 2 בחזקת

static bool activeLow = false;  // מצב האקטיב הדפולטיבי הוא HIGH

void pumpSetActiveLow(bool en) {
    activeLow = en;
}

void pumpInit(int pin_pump) {
    int PWM_CHANNEL = (pin_pump >= 0) ? (pin_pump % 16) : 0;

    ledcSetup(PWM_CHANNEL, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttachPin(pin_pump, PWM_CHANNEL);  // connect between pin to PWM. after that the control is on the PWM channel

    // מתחילים כבוי 
    ledcWrite(PWM_CHANNEL, activeLow ? MAX_DUTY : 0);// מאתחל pwm כבוי
                                                      // אם activeLow אמת אז ערך התחלתי maxDuty
                                                      // אחרת ערך התחלתי אפס
    Serial.printf("[HW] Pump init on GPIO %d (Channel %d)\n", pin_pump, PWM_CHANNEL);
}

void pumpOn(int pin_pump) {
    int PWM_CHANNEL = (pin_pump >= 0) ? (pin_pump % 16) : 0;
    ledcWrite(PWM_CHANNEL, activeLow ? 0 : MAX_DUTY);
    Serial.printf("[HW] Pump ON -> GPIO %d (Channel %d)\n", pin_pump, PWM_CHANNEL);
}

void pumpOff(int pin_pump) {
    int PWM_CHANNEL = (pin_pump >= 0) ? (pin_pump % 16) : 0;
    ledcWrite(PWM_CHANNEL, activeLow ? MAX_DUTY : 0);
    Serial.printf("[HW] Pump OFF -> GPIO %d (Channel %d)\n", pin_pump, PWM_CHANNEL);
}

void pumpToggle(int pin_pump) {
    if (pumpIsOn(pin_pump)) pumpOff(pin_pump);
    else           pumpOn(pin_pump);
}

bool pumpIsOn(int pin_pump) {
    int PWM_CHANNEL = (pin_pump >= 0) ? (pin_pump % 16) : 0;
    return ledcRead(PWM_CHANNEL) > 0;
}

void setPumpPercent(int pin_pump, uint8_t pct) {
    int PWM_CHANNEL = (pin_pump >= 0) ? (pin_pump % 16) : 0;

    if (pct > 100) pct = 100;
    uint32_t duty = (pct * MAX_DUTY) / 100u;

    if (activeLow) duty = MAX_DUTY - duty;
    
    ledcWrite(PWM_CHANNEL, duty);
}
