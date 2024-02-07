#ifndef WiFiWebServer_h
#define WiFiWebServer_h

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>  // Include the ArduinoJson library for easy JSON manipulation

extern AsyncWebServer server;
extern AsyncEventSource events; // Declare an AsyncEventSource for SSE

extern const char* PARAM_INPUT_1;
extern const char* PARAM_INPUT_2;
extern const char* PARAM_INPUT_3;
extern const char* PARAM_INPUT_4;

void initWiFi(const String& ssid, const String& password, const String& ip, const String& gateway);
void startServer();
void notifyClients(uint8_t val);  // Simplified to just send the uint8_t value

#endif
