#include "plants.h"

Plant g_plants[] = {
  // id, name, moisturePin, pumpPin, pumpActiveHigh, dryThreshold(%), wetThresh(%), last watering
  { 0, "Basil",    34,          23,   true,           30,    45, "-"},
 // { 35,          26,      "Mint",    true,           2100 },
 // { 32,          27,      "Rosemary",true,           2300 },
};

const size_t g_plants_count = sizeof(g_plants) / sizeof(g_plants[0]); // גודל הביטים של המערך הכולל חלקי גודל הביטים של איבר אחד
