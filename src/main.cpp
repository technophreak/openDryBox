// Load required libraries
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Preferences.h>

// Load Settings Definition
#include "allSettings.h"

// Load Web Server
#include "webServer.h"

// Load Application Functions
#include "appFunctions.h"

// Program constants definition
const char* PROGRAM_NAME = "openDryBox";
const char* PROGRAM_VERSION = "v0.0.5";

// Application variables definition
Preferences myPreferences;
WebServer* webServer = nullptr;
bool wifiServiceStarted = false;
bool otaServiceStarted = false;
uint64_t sensor0ReadMillis = millis();
String sensor0Temperature = "0";
String sensor0Humidity = "0";
bool outputHeat = false;
bool outputFan = false;

void setup() {

  // Init Serial 
  Serial.begin(921600);
  while (!Serial) {
    delay(10); // Wait for serial port to connect
  }

  // Display program version
  Serial.println("\n\nProgram: " + String(PROGRAM_NAME) + " - " + String(PROGRAM_VERSION));
 
  // Initialize Settings Definition
  objSettings = initSettingsDefinition(allSettings);

  // Load Preferences
  loadPreferences();

  // Connect to Wifi
  connectWifi();

  if (wifiServiceStarted) {

    // Init Rest Server
    webServer = new WebServer(objSettings, &myPreferences);

    // Init OTA Updates
    otaUpdatesInit();

  }

}

void loop() {

  // Handle client requests and OTA updates
  if (wifiServiceStarted) {
    webServer->handleClient();
    ArduinoOTA.handle();
  }

}