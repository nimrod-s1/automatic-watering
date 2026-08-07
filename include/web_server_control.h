// web_server_control.h
#pragma once
#include <stdint.h>

// אתחול רשת + WebSocket.
// אם STA לא מתחבר, נפתח AP עם ברירות המחדל או הערכים שתעביר.
void webServerBegin(const char* staSsid,
                    const char* staPass,
                    const char* apSsid = "Plant-AP",
                    const char* apPass = "12345678");

// לעבד אירועי WebSocket בכל loop()
void webServerLoop();

// גישה/עדכון לסף היובש (נשמר ב-NVS במימוש הנוכחי)
uint8_t webServerGetThreshold();
void    webServerSetThreshold(uint8_t thr);
