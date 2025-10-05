#include <Arduino.h>
#include <Preferences.h>
#include "driver/adc.h"
#include "sensor_control.h"
#include "pump_control.h" 
#include "wifi_control.h"
#include "plants.h"
#include "time.h"

const unsigned long CHECK_INTERVAL_MS = 10UL * 60UL * 1000UL; // 10 דק'
const unsigned long WATER_STEP_MS = 200UL;   // כל כמה לבדוק תוך כדי השקיה
const unsigned long MAX_ON_MS = 5UL * 1000UL;       // 5 שניות


// ====== קבועים ושמות אחסון ======
constexpr char WIFI_SSID[] = "YOUR_HOME_WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "YOUR_HOME_WIFI_PASSWORD"; 

// שרת זמן (NTP)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 2 * 3600;  // שעון ישראל (GMT+2)
const int daylightOffset_sec = 3600;  // הוספת שעה בקיץ


// ---- פונקציות גישה לזכרון פלאש ----
static Preferences prefs;   //  גישה לזכרון פלאש

void saveWateringTime(Plant& pl) {
  struct tm timeinfo;
  const String key = "plant_" + String(pl.id);

  if (getLocalTime(&timeinfo)) {
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
    prefs.putString(key.c_str(), buffer);
    pl.lastWatering = buffer;
  } else {
    // סימון מפורש שנכשל לקבל זמן
    const char* FAIL = "NTP_FAIL";
    prefs.putString(key.c_str(), FAIL);
    pl.lastWatering = FAIL;
    Serial.println("saveWateringTime: NTP failed, wrote NTP_FAIL");
  }
}


String getWateringTime(int plant_id){
  String key = "plant_" + String(plant_id);

  String lastWatering = prefs.getString(key.c_str(), "-");
  return lastWatering;
}


void setup() {   // put your setup code here, to run once:
  // הפעלת תקשורת סיריאלית במהירות 115200
  Serial.begin(115200);
  delay(200);

  prefs.begin("watering", false);  
  
  for (size_t i = 0; i < g_plants_count; ++i) {   
      Plant& pl = g_plants[i];  // &- להתעסק עם האובייקט עצמו ולא עותק
      
      // Moisture Sensor ADC Setting
      sensorInit(pl.moisturePin);
      pumpInit(pl.pumpPin);

      // קבלת זמן השקייה אחרון מהפלאש
      pl.lastWatering = getWateringTime(pl.id);
  }
    
  wifiControlBegin(WIFI_SSID, WIFI_PASSWORD); 
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.println("Boot done! start");
}

void loop() { // put your main code here, to run repeatedly:
  wifiControlLoop();

  for (size_t i = 0; i < g_plants_count; ++i) {
    Plant& pl = g_plants[i];  // &- להתעסק עם האובייקט עצמו ולא עותק

    if (pl.dryThreshold >= getMoisturePercent(pl.moisturePin)) {  // צריך להשקות
      const unsigned long startedAt = millis();
      
      bool watering = true;
      pumpOn(pl.pumpPin);

      while (watering)
      {
        delay(WATER_STEP_MS);

        if (getMoisturePercent(pl.moisturePin) >= pl.wetThresh){
          watering = false;
        }

        const unsigned long elapsed = millis() - startedAt;
        if (elapsed >= MAX_ON_MS){
          watering = false;
        }
      }
      pumpOff(pl.pumpPin);
      saveWateringTime(pl);
    }
  }

  delay(CHECK_INTERVAL_MS);
}









