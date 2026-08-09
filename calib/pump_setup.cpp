// הקובץ נועד לבחון את יציאת המתח מהפין שנבחר לחיבור ה-MOSFET והמשאבה דרך הטרמינל

#include <Arduino.h>

int currentPin = 23;   // פין ברירת מחדל לבדיקה (GPIO25 לדוגמה)
bool pinState = false;
int pwmDuty = 255;     // ערך PWM בין 0 ל-255
bool usePwm = false;
int pwmChannel = 0;

void printMenu() {
  Serial.println("\n==========================================");
  Serial.println("         Pump & MOSFET Setup / Calib      ");
  Serial.println("==========================================");
  Serial.printf("Current Target GPIO Pin: %d\n", currentPin);
  Serial.printf("Current Pin State: %s\n", pinState ? "HIGH (3.3V)" : "LOW (0V)");
  Serial.println("------------------------------------------");
  Serial.println("Commands:");
  Serial.println("  p <num>   - Set target GPIO pin (e.g. 'p 25' or 'p 18')");
  Serial.println("  1 / h     - Set Pin HIGH (Output 3.3V / Turn MOSFET ON)");
  Serial.println("  0 / l     - Set Pin LOW  (Output 0V / Turn MOSFET OFF)");
  Serial.println("  t         - Toggle Pin state (HIGH <-> LOW)");
  Serial.println("  v <0-255> - Set PWM Duty Cycle (e.g. 'v 128' for 50% duty)");
  Serial.println("  ? / menu  - Print this menu again");
  Serial.println("==========================================\n");
}

void updatePinOutput() {
  pinMode(currentPin, OUTPUT);
  if (usePwm) {
    ledcSetup(pwmChannel, 5000, 8);
    ledcAttachPin(currentPin, pwmChannel);
    ledcWrite(pwmChannel, pinState ? pwmDuty : 0);
    Serial.printf("[MOSFET SETUP] GPIO %d set to PWM Duty: %d (State: %s)\n",
                  currentPin, pinState ? pwmDuty : 0, pinState ? "ON" : "OFF");
  } else {
    ledcDetachPin(currentPin);
    digitalWrite(currentPin, pinState ? HIGH : LOW);
    Serial.printf("[MOSFET SETUP] GPIO %d set to Digital %s (%s)\n",
                  currentPin, pinState ? "HIGH" : "LOW", pinState ? "3.3V" : "0V");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(currentPin, OUTPUT);
  digitalWrite(currentPin, LOW);

  printMenu();
}

void processCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd.startsWith("p ") || cmd.startsWith("P ")) {
    int newPin = cmd.substring(2).toInt();
    if (newPin >= 0 && newPin <= 39) {
      // Turn off current pin before switching
      digitalWrite(currentPin, LOW);
      currentPin = newPin;
      pwmChannel = currentPin % 16;
      Serial.printf(">> Changed target pin to GPIO %d\n", currentPin);
      updatePinOutput();
    } else {
      Serial.println(">> Error: Invalid GPIO pin number (0-39)");
    }
  }
  else if (cmd == "1" || cmd == "h" || cmd == "H" || cmd == "high" || cmd == "HIGH") {
    pinState = true;
    usePwm = false;
    updatePinOutput();
  }
  else if (cmd == "0" || cmd == "l" || cmd == "L" || cmd == "low" || cmd == "LOW") {
    pinState = false;
    usePwm = false;
    updatePinOutput();
  }
  else if (cmd == "t" || cmd == "T" || cmd == "toggle") {
    pinState = !pinState;
    usePwm = false;
    updatePinOutput();
  }
  else if (cmd.startsWith("v ") || cmd.startsWith("V ")) {
    int val = cmd.substring(2).toInt();
    if (val < 0) val = 0;
    if (val > 255) val = 255;
    pwmDuty = val;
    usePwm = true;
    pinState = (pwmDuty > 0);
    updatePinOutput();
  }
  else if (cmd == "?" || cmd == "menu" || cmd == "help") {
    printMenu();
  }
  else {
    Serial.printf(">> Unknown command: '%s'. Type '?' or 'menu' for options.\n", cmd.c_str());
  }
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    processCommand(input);
  }
}
