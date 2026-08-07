#include "plants.h"
#include "sensor_control.h"
#include "pump_control.h"
#include <Preferences.h>
#include <ArduinoJson.h>

std::vector<Plant> g_plants;
static Preferences prefs;
static const char* PREFS_NAMESPACE = "plant_store";
static const char* PREFS_KEY = "plants_data";

void savePlantsToNVS() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& p : g_plants) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"] = p.id;
        obj["name"] = p.name;
        obj["moisturePin"] = p.moisturePin;
        obj["pumpPin"] = p.pumpPin;
        obj["pumpActiveHigh"] = p.pumpActiveHigh;
        obj["dryThreshold"] = p.dryThreshold;
        obj["wetThresh"] = p.wetThresh;
        obj["lastWatering"] = p.lastWatering;
    }
    String jsonStr;
    serializeJson(doc, jsonStr);
    
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putString(PREFS_KEY, jsonStr);
    prefs.end();
}

void initPlantStorage() {
    g_plants.clear();
    prefs.begin(PREFS_NAMESPACE, true);
    String jsonStr = prefs.getString(PREFS_KEY, "");
    prefs.end();

    if (jsonStr.length() > 0) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, jsonStr);
        if (!err && doc.is<JsonArray>()) {
            for (JsonObject obj : doc.as<JsonArray>()) {
                Plant p;
                p.id = obj["id"] | 0;
                p.name = obj["name"] | "Plant";
                p.moisturePin = obj["moisturePin"] | 34;
                p.pumpPin = obj["pumpPin"] | 23;
                p.pumpActiveHigh = obj["pumpActiveHigh"] | true;
                p.dryThreshold = obj["dryThreshold"] | 30;
                p.wetThresh = obj["wetThresh"] | 45;
                p.lastWatering = obj["lastWatering"] | "-";
                
                sensorInit(p.moisturePin);
                pumpInit(p.pumpPin);
                g_plants.push_back(p);
            }
        }
    }

    // אם הזיכרון ריק (אתחול ראשון), ניצור צמח ברירת מחדל
    if (g_plants.empty()) {
        Plant defaultPlant = { 0, "Basil", 34, 23, true, 30, 45, "-" };
        sensorInit(defaultPlant.moisturePin);
        pumpInit(defaultPlant.pumpPin);
        g_plants.push_back(defaultPlant);
        savePlantsToNVS();
    }
}

bool addPlant(const String& name, uint8_t moisturePin, uint8_t pumpPin, uint8_t dryThreshold, uint8_t wetThresh, bool pumpActiveHigh) {
    uint8_t newId = 0;
    for (const auto& p : g_plants) {
        if (p.id >= newId) {
            newId = p.id + 1;
        }
    }
    Plant p;
    p.id = newId;
    p.name = name.length() > 0 ? name : ("Plant " + String(newId));
    p.moisturePin = moisturePin;
    p.pumpPin = pumpPin;
    p.pumpActiveHigh = pumpActiveHigh;
    p.dryThreshold = dryThreshold;
    p.wetThresh = wetThresh;
    p.lastWatering = "-";

    sensorInit(p.moisturePin);
    pumpInit(p.pumpPin);

    g_plants.push_back(p);
    savePlantsToNVS();
    return true;
}

bool removePlant(uint8_t id) {
    for (auto it = g_plants.begin(); it != g_plants.end(); ++it) {
        if (it->id == id) {
            pumpOff(it->pumpPin);
            g_plants.erase(it);
            savePlantsToNVS();
            return true;
        }
    }
    return false;
}

bool updatePlant(uint8_t id, const String& name, uint8_t moisturePin, uint8_t pumpPin, uint8_t dryThreshold, uint8_t wetThresh, bool pumpActiveHigh) {
    for (auto& p : g_plants) {
        if (p.id == id) {
            p.name = name;
            p.moisturePin = moisturePin;
            p.pumpPin = pumpPin;
            p.dryThreshold = dryThreshold;
            p.wetThresh = wetThresh;
            p.pumpActiveHigh = pumpActiveHigh;

            sensorInit(p.moisturePin);
            pumpInit(p.pumpPin);

            savePlantsToNVS();
            return true;
        }
    }
    return false;
}

Plant* getPlantById(uint8_t id) {
    for (auto& p : g_plants) {
        if (p.id == id) return &p;
    }
    return nullptr;
}
