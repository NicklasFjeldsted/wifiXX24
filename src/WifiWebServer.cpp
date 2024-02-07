/**
 * @file WifiWebServer.cpp
 * @brief Implementation file for the WiFi web server.
 * 
 * This file contains the implementation of the WiFi web server, which is responsible for handling
 * web requests and serving web pages. It includes functions for initializing the WiFi connection,
 * starting the server, and notifying connected clients with JSON data.
 */

#include "WiFiWebServer.h"
#include "SPIFFSManager.h" // For file operations

// Initialize server on port 80
AsyncWebServer server(80);
AsyncEventSource events("/events");

const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";
const char *PARAM_INPUT_3 = "ip";
const char *PARAM_INPUT_4 = "gateway";

/**
 * @brief Initializes the WiFi connection with the provided parameters.
 * 
 * This function configures the WiFi mode, IP address, and gateway based on the provided parameters.
 * It then attempts to connect to the WiFi network using the provided SSID and password.
 * If the connection is successful, it prints the local IP address.
 * 
 * @param ssid The SSID of the WiFi network.
 * @param password The password for the WiFi network.
 * @param ip The desired IP address for the device.
 * @param gateway The gateway IP address for the network.
 */
void initWiFi(const String &ssid, const String &password, const String &ip, const String &gateway)
{
    if (ssid.isEmpty() || ip.isEmpty())
    {
        Serial.println("Undefined SSID or IP address.");
        return;
    }

    WiFi.mode(WIFI_STA);
    IPAddress localIP, localGateway, subnet(255, 255, 0, 0);
    localIP.fromString(ip);
    localGateway.fromString(gateway);

    if (!WiFi.config(localIP, localGateway, subnet))
    {
        Serial.println("STA Failed to configure");
        return;
    }

    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Connecting to WiFi...");

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
    {
        delay(100);
        Serial.print(".");
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Failed to connect.");
    }
    else
    {
        Serial.println("Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    }
}


/**
 * @brief Starts the web server and sets up the server routes and handlers.
 * 
 * This function initializes the server and adds the necessary routes and handlers for serving web pages.
 * It also starts the server and prints a message to the serial monitor indicating that the server has started.
 */
void startServer()
{
    // Setup Server-Sent Events (SSE)
    server.addHandler(&events);

    // Define your server routes and handlers here
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", String(), false);
    });

    server.serveStatic("/", SPIFFS, "/");

    // Add more routes as needed

    Serial.println("Server started.");
    server.begin();
}

/**
 * @brief Notifies all connected clients with a JSON object containing the given value.
 * 
 * @param val The value to be included in the JSON object.
 */
void notifyClients(uint8_t val) {
    // Construct a simple JSON object to hold the data
    JsonDocument doc;
    doc["val"] = val; // Add the ECG value
    // Convert the JSON object to a string
    String output;
    serializeJson(doc, output);
    // Send the data to all connected clients
    events.send(output.c_str(), "value", millis());
}