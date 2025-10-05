// wifi_control.cpp
// ESP32 + Arduino framework
// WebSocket על פורט 81 לשליחת/קבלת מצב מול ה-HTML, כולל שמירה ב-NVS

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>   // arduinoWebSockets
#include <Preferences.h>
#include <ArduinoJson.h> 
#include "sensor_control.h"
#include "pump_control.h" 
#include "wifi_control.h"
#include "plants.h"


//==================== פונקציות שנלקחות מתקיות אחרות ========================
    // לחות נוכחית באחוזים (0 = רטוב, 100 = יבש)
    //  int  getMoisturePercent();    from sensor_control.h
    // שליטת משאבה
    // void pumpOn();                from pump_control.h
    // void pumpOff();
    // void pumpToggle();
    // bool pumpIsOn();
    // void pumpSetPin(uint8_t gpio);    חשוב לממש במודול המשאבה
    // uint8_t pumpGetPin();             החזרת ה-GPIO הנוכחי של המשאבה


static const uint16_t WS_PORT = 81;

// ====== אובייקטים גלובליים פנימיים ======
static WebSocketsServer ws(WS_PORT);
static Preferences prefs;

static uint8_t g_threshold = 60;   // ברירת מחדל לחות
//static uint8_t g_pumpPin   = 23;   // ברירת מחדל פין

// ====== עזר ======
static String localIpString() {  // פונקצייה שמחזירה כסטרינג את האיי פי של הבקר
  if (WiFi.getMode() & WIFI_MODE_STA) {  // אם מחובר לראוטר חיצוני -STA
    IPAddress ip = WiFi.localIP();
    if (ip[0] != 0) return ip.toString();
  }
  if (WiFi.getMode() & WIFI_MODE_AP) {  // אם הבקר פועל כנקודה חמה
    return WiFi.softAPIP().toString();
  }
  return String("-");
}

// בונה JSON מצב ושולח לכל הלקוחות
static void wsBroadcastStatus() {
    const String ip = localIpString();  // גלובלי לכל ההודעה

    JsonDocument doc;
    doc["ip"] = ip;

    JsonArray arr = doc["plants"].to<JsonArray>();
    for (size_t i = 0; i < g_plants_count; ++i) {
      const Plant& pl = g_plants[i];
      JsonObject o = arr.add<JsonObject>();
      o["id"] = pl.id;
      o["name"] = pl.name;
      o["pump"] = pumpIsOn(pl.pumpPin);
      o["dryness"] = getMoisturePercent(pl.moisturePin);
      o["threshold"] = pl.dryThreshold;
      o["moisture_pin"] = pl.moisturePin;
      o["pump_pin"] = pl.pumpPin;
      o["last_watering"] = pl.lastWatering;
      // אפשר להוסיף עוד שדות לפי הצורך (gpio, adc, וכו')
    }

    String out; serializeJson(doc, out);
    ws.broadcastTXT(out);
}


// =============== מה קורה כשלקוח מתחבר לווב סוקט ====================

static void onWsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len) { 
  if (type == WStype_CONNECTED) {  // כאשר מתבצעת ההתחברות
    wsBroadcastStatus();  // פונקציית שינוי הUI
  }

  else if (type == WStype_TEXT) {  // אם התקבל טקסט
    JsonDocument doc;  // יוצרים JSON
    DeserializationError err = deserializeJson(doc, payload, len);  // מנסים להעביר את הפיילוד לגייסון (דוק)
    if (err) {
      ws.sendTXT(num, "{\"ok\":false,\"err\":\"bad_json\"}");
      return;
    }

    const char* cmd = doc["cmd"] | "";
    
    if (strcmp(cmd, "pump") == 0) {  // קבלת פקודה להדליק או לכבות משאבה
      const char* v = doc["value"];
      if (strcmp(v, "on") == 0) pumpOn(doc["pump_pin"]);
      else if (strcmp(v, "off") == 0) pumpOff(doc["pump_pin"]);
      else if (strcmp(v, "toggle") == 0) pumpToggle(doc["pump_pin"]);
      wsBroadcastStatus();
    }
    //else if (strcmp(cmd, "set_pump_pin") == 0) {
    //  int p = doc["value"];
    //  if (p >= 0 && p <= 39) {
    //    savePumpPin((uint8_t)p);
    //    wsBroadcastStatus();
    //  } else {
    //    ws.sendTXT(num, "{\"ok\":false,\"err\":\"bad_pin\"}");
    //  }
    //}
    else {
      ws.sendTXT(num, "{\"ok\":false,\"err\":\"unknown_cmd\"}");
    }
  }
}


// =============== חיבור לוויפיי =========================

void wifiControlBegin(const char* staSsid, const char* staPass,  // שם ראוטר וסיסמא לראוטר חיצוני
                      const char* apSsid, const char* apPass) {  // שם נקודת גלישה וסיסמה
  

  WiFi.mode(WIFI_STA);  // מנסה להתחבר לראוטר חיצוני
  WiFi.begin(staSsid, staPass);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 4000) {  // מנסה להתחבר כל 200 מילישניות, בחלון של 4 שניות
    delay(200);
  }
  if (WiFi.status() != WL_CONNECTED) {  // אם נכשל להתחבר לראוטר חיצוני
    Serial.println("WiFi: STA connect failed. Starting AP mode...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid, apPass);
  }
  else{
    Serial.printf("WiFi: STA connected");
  }

  ws.begin();
  ws.onEvent(onWsEvent);
  Serial.printf("WS: listening");
}

void wifiControlLoop() {
  ws.loop();  //  דואג לכך שהווב סוקט ממשיך לחכות לקריאות
}




       
