#pragma once
#include <Arduino.h>

// מבנה שמייצג צמח יחיד
struct Plant {
    uint8_t id;
    String name;        // שם אופציונלי לתצוגה
    uint8_t moisturePin;     
    uint8_t pumpPin;         
    bool pumpActiveHigh;     // true אם HIGH מדליק משאבה, false אם LOW מדליק (היפוך)
    uint8_t dryThreshold;   // סף "יבש" (%)
    uint8_t wetThresh;
    String lastWatering;
};

// הצהרות על המשתנים הגלובליים (אין כאן הקצאה!)
extern Plant g_plants[];
extern const size_t g_plants_count;
