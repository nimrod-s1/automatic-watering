#include <Arduino.h>
#include <Preferences.h>
#include "driver/adc.h"
#include "sensor_control.h"
#include "pump_control.h" 
#include "web_server_control.h"
#include "plants.h"
#include "time.h"
#include <WiFi.h>

const unsigned long CHECK_INTERVAL_MS = 30UL * 60UL * 1000UL; // לבדיקה: בדיקה כל חצי שעה
const unsigned long WATER_STEP_MS = 200UL;   // כל כמה לבדוק תוך כדי השקיה
const unsigned long MAX_ON_MS = 10UL * 1000UL;       // 10 שניות

unsigned long g_lastCheckTime = 0; // משתנה למעקב אחר זמן בדיקה אחרון


// ====== קבועים ושמות אחסון ======
constexpr char WIFI_SSID[] = "";
constexpr char WIFI_PASSWORD[] = ""; 

// שרת זמן (NTP)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 2 * 3600;  // שעון ישראל (GMT+2)
const int daylightOffset_sec = 3600;  // הוספת שעה בקיץ


void saveWateringTime(Plant& pl) {
  struct tm timeinfo;

  if (getLocalTime(&timeinfo)) {
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
    pl.lastWatering = buffer;
  } else {
    // סימון מפורש שנכשל לקבל זמן
    const char* FAIL = "NTP_FAIL";
    pl.lastWatering = FAIL;
    Serial.println("saveWateringTime: NTP failed, wrote NTP_FAIL");
  }
  savePlantsToNVS();
}


void setup() {   // put your setup code here, to run once:
  // הפעלת תקשורת סיריאלית במהירות 115200
  Serial.begin(115200);
  delay(200);

  // אתחול וטעינת רשימת הצמחים הדינמית מ-NVS
  initPlantStorage();
    
  webServerBegin(WIFI_SSID, WIFI_PASSWORD); 
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // הדפסת IP בטיחותית
  if (WiFi.getMode() & WIFI_MODE_STA) {
    Serial.printf("MAIN: Connected IP: %s\n", WiFi.localIP().toString().c_str());
  } else if (WiFi.getMode() & WIFI_MODE_AP) {
    Serial.printf("MAIN: AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  }

  Serial.println("Boot done! start");
}

void loop() { // put your main code here, to run repeatedly:
  // **חובה: חייב לרוץ כל הזמן כדי לטפל בבקשות רשת**
  webServerLoop(); 

  // **בדיקת השקיה באמצעות טיימר לא חוסם**
  if (millis() - g_lastCheckTime >= CHECK_INTERVAL_MS) {
    g_lastCheckTime = millis();
    Serial.printf("\n--- Scheduled Check (%u plants in system) ---\n", g_plants.size());

    if (g_plants.empty()) {
      Serial.println("[Check] No plants found in list. Add a plant from Web UI.");
    }

    for (size_t i = 0; i < g_plants.size(); ++i) {
      Plant& pl = g_plants[i];
      int currentMoisture = getMoisturePercent(pl.moisturePin);

      Serial.printf("[Check] Plant ID %d '%s' (M-Pin %d, P-Pin %d): Moisture = %d%% | Dry Thresh = %d%%\n",
                    pl.id, pl.name.c_str(), pl.moisturePin, pl.pumpPin, currentMoisture, pl.dryThreshold);

      if (currentMoisture <= pl.dryThreshold) {  // צריך להשקות
        Serial.printf("--> Plant '%s' IS DRY! Starting watering...\n", pl.name.c_str());
        const unsigned long startedAt = millis();
        
        bool watering = true;
        pumpOn(pl.pumpPin);

        while (watering)
        {
          webServerLoop(); // מאפשר טיפול ברשת במהלך ההשקיה
          delay(WATER_STEP_MS);

          int m = getMoisturePercent(pl.moisturePin);
          if (m >= pl.wetThresh){
            watering = false;
            Serial.printf("--> Finished watering! Moisture reached %d%%\n", m);
          }

          const unsigned long elapsed = millis() - startedAt;
          if (elapsed >= MAX_ON_MS){
            watering = false;
            Serial.println("--> MAX_ON_MS safety timeout reached.");
          }
        }
        pumpOff(pl.pumpPin);
        saveWateringTime(pl);
        Serial.printf("Watering Plant ID %d FINISHED.\n", pl.id);
      } else {
        Serial.printf("--> Plant '%s' is moist enough (no watering needed).\n", pl.name.c_str());
      }
    }
  }
}