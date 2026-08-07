#pragma once
#include <Arduino.h>
#include <vector>

// מבנה שמייצג צמח יחיד
struct Plant {
    uint8_t id;
    String name;             // שם לתצוגה
    uint8_t moisturePin;     
    uint8_t pumpPin;         
    bool pumpActiveHigh;     // true אם HIGH מדליק משאבה, false אם LOW מדליק (היפוך)
    uint8_t dryThreshold;    // סף "יבש" (%)
    uint8_t wetThresh;       // סף "רטוב" (%)
    String lastWatering;
};

// רשימה דינמית של הצמחים
extern std::vector<Plant> g_plants;

// פונקציות ניהול צמחים וזיכרון NVS
void initPlantStorage();
void savePlantsToNVS();
bool addPlant(const String& name, uint8_t moisturePin, uint8_t pumpPin, uint8_t dryThreshold, uint8_t wetThresh, bool pumpActiveHigh = true);
bool removePlant(uint8_t id);
bool updatePlant(uint8_t id, const String& name, uint8_t moisturePin, uint8_t pumpPin, uint8_t dryThreshold, uint8_t wetThresh, bool pumpActiveHigh = true);
Plant* getPlantById(uint8_t id);
