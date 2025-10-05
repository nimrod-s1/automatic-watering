//  הקובץ נועד למדוד את טווח הערכים הדיגיטלים של הסנסור לחות

#include <Arduino.h>
#include "driver/adc.h"

const int PIN_SOIL = 34;   // שים כאן את ה-ADC שבחרת (GPIO34 לדוגמה)
int dryValue = -1;  // 3115
int wetValue = -1;  // 955

int readAveragedADC(int pin, int samples = 100) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return (int)(sum / samples);
}

int percentFromRaw(int raw, int dry, int wet) {
  // low ADC - wet;    high ADC - dry
  long p = (long)(raw - wet) * 100 / (long)(dry - wet);
  if (p < 0) p = 0;
  if (p > 100) p = 100;
  return (int)p;
}


void setup() {   // put your setup code here, to run once:
  // הפעלת תקשורת סיריאלית במהירות 115200
  Serial.begin(115200);
  delay(200);

  // ADC Setting
  analogReadResolution(12); // 12 bits (0-4095)
  analogSetPinAttenuation(PIN_SOIL, ADC_11db);  // Voltage range (ADC_11db: 0-3.3V)

  Serial.println("\nSoil Sensor Calibration");
  Serial.println("-------- הוראות --------");
  Serial.println("1) 'd' = מדידת יבש (באוויר או אדמה יבשה)");
  Serial.println("2) 'w' = מדידת רטוב (בכוס מים)");
  Serial.println("3) אחרי שתי המדידות, אחשב אחוזים.");
  Serial.println("-------------------------\n");
}

void loop() { // put your main code here, to run repeatedly:
  int raw = readAveragedADC(PIN_SOIL, 10);
  Serial.print("Raw=");
  Serial.print(raw);

   if (dryValue >= 0) {
    Serial.print("  dry=");
    Serial.print(dryValue);
  }

  if (wetValue >= 0) {
    Serial.print("  wet=");
    Serial.print(wetValue);
  }

  if (dryValue >= 0 && wetValue >= 0 && dryValue != wetValue) {
    int perc = percentFromRaw(raw, dryValue, wetValue);
    Serial.print("  ~");
    Serial.print(perc);
    Serial.print("%");
  }
  Serial.println();


  // טיפול בקלט מהמוניטור
  if (Serial.available()) {  // אם יש קלט  (מחזיר את מספר התווים של הקלט)
    char c = Serial.read();
    if (c == 'd' || c == 'D') {
      Serial.println("מודד יבש... מחכה לייצוב 2 שניות");
      delay(2000);
      dryValue = readAveragedADC(PIN_SOIL);
      Serial.print("dryValue = ");
      Serial.println(dryValue);
    } 
    else if (c == 'w' || c == 'W') {
      Serial.println("מודד רטוב... מחכה לייצוב 2 שניות");
      delay(2000);
      wetValue = readAveragedADC(PIN_SOIL);
      Serial.print("wetValue = ");
      Serial.println(wetValue);
    }
  }

  delay(300);
}



