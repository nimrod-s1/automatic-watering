// html_backend.cpp (web_server module)
// ESP32 + Arduino framework
// Backend פשוט ורזה לניהול HTTP ו-WebSocket

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h> 
#include <WebServer.h>

#include "sensor_control.h"
#include "pump_control.h" 
#include "web_server_control.h"
#include "plants.h"
#include "html_data.h"

static const uint16_t WS_PORT = 81; 
static WebSocketsServer ws(WS_PORT);
static WebServer server(80);

// מחזיר את כתובת ה-IP הפעילה
static String localIpString() {
  if (WiFi.getMode() & WIFI_MODE_STA) {
    IPAddress ip = WiFi.localIP();
    if (ip[0] != 0) return ip.toString();
  }
  if (WiFi.getMode() & WIFI_MODE_AP) { // אם הבקר פועל כנקודה חמה
    return WiFi.softAPIP().toString();
  }
  return String("-");
}

// בונה JSON מצב ושולח לכל הלקוחות
static void wsBroadcastStatus() {
    JsonDocument doc;
    doc["ip"] = localIpString();

    JsonArray arr = doc["plants"].to<JsonArray>();
    for (const auto& pl : g_plants) {
      JsonObject o = arr.add<JsonObject>();
      o["id"] = pl.id;
      o["name"] = pl.name;
      o["pump"] = pumpIsOn(pl.pumpPin);
      o["dryness"] = getMoisturePercent(pl.moisturePin);
      o["threshold"] = pl.dryThreshold;
      o["wet_thresh"] = pl.wetThresh;
      o["moisture_pin"] = pl.moisturePin;
      o["pump_pin"] = pl.pumpPin;
      o["last_watering"] = pl.lastWatering;
    }

    String out; 
    serializeJson(doc, out);
    ws.broadcastTXT(out);
}

// =============== מה קורה כשלקוח מתחבר לווב סוקט ====================

static void onWsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len) { 
  if (type == WStype_CONNECTED) {  // כאשר מתבצעת ההתחברות
    wsBroadcastStatus();  // פונקציית שינוי הUI
  }
  else if (type == WStype_TEXT) {  // אם התקבל טקסט (פקודות)
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, len); // מנסים להעביר את הפיילוד לגייסון (דוק)
    if (err) return;

    const char* cmd = doc["cmd"] | "";
    
    if (strcmp(cmd, "pump") == 0) {  // קבלת פקודה להדליק או לכבות משאבה
      uint8_t pin = doc["pump_pin"];
      const char* val = doc["value"] | "";
      Serial.printf("[WS] Received pump command for GPIO %d: %s\n", pin, val);

      if (strcmp(val, "on") == 0) pumpOn(pin);
      else if (strcmp(val, "off") == 0) pumpOff(pin);
      else if (strcmp(val, "toggle") == 0) pumpToggle(pin);

      wsBroadcastStatus();
    }
    else if (strcmp(cmd, "add_plant") == 0) {
      String name = doc["name"] | "";
      uint8_t moisturePin = doc["moisture_pin"] | 34;
      uint8_t pumpPin = doc["pump_pin"] | 23;
      uint8_t dryThreshold = doc["threshold"] | 30;
      uint8_t wetThresh = doc["wet_thresh"] | 45;

      addPlant(name, moisturePin, pumpPin, dryThreshold, wetThresh);
      wsBroadcastStatus();
    }
    else if (strcmp(cmd, "delete_plant") == 0) {
      uint8_t id = doc["id"];
      removePlant(id);
      wsBroadcastStatus();
    }
    else if (strcmp(cmd, "update_plant") == 0) {
      uint8_t id = doc["id"];
      String name = doc["name"] | "";
      uint8_t moisturePin = doc["moisture_pin"] | 34;
      uint8_t pumpPin = doc["pump_pin"] | 23;
      uint8_t dryThreshold = doc["threshold"] | 30;
      uint8_t wetThresh = doc["wet_thresh"] | 45;

      updatePlant(id, name, moisturePin, pumpPin, dryThreshold, wetThresh);
      wsBroadcastStatus();
    }
  }
}

// הגשת עמוד ה-HTML מזיכרון Flash ישירות
static void handleRoot() {
  server.send_P(200, "text/html", HTML_CONTENT); 
}

// אתחול חיבור Wi-Fi והשרתים
void webServerBegin(const char* staSsid, const char* staPass,
                    const char* apSsid, const char* apPass) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(staSsid, staPass);
  
  unsigned long t0 = millis();
   // מנסה להתחבר כל 200 מילישניות בחלון של 8 שניות
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) {
    delay(200);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) { // אם נכשל להתחבר לראוטר חיצוני
    Serial.println("\nWiFi: STA connect failed. Starting AP mode...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid, apPass);
  } else {
    Serial.println("\nWiFi: STA connected!");
  }

  // אתחול mDNS - מאפשר גישה מהטלפון/מחשב דרך http://smartplant.local
  if (MDNS.begin("smartplant")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS responder started: http://smartplant.local");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", HTTP_GET, handleRoot);  // מגדירים את הפונקציה שתטפל ב-IP הבסיסי
  server.begin();

  ws.begin();
  ws.onEvent(onWsEvent);
}

// לולאה ראשית לתקשורת
void webServerLoop() {
  ws.loop();   //  דואג לכך שהווב סוקט ממשיך לחכות לקריאות
  server.handleClient();
}
