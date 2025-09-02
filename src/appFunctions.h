#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include "allSettings.h"

// Forward declaration
class WebServer;

// Program constants (defined in main.cpp)
extern const char* PROGRAM_NAME;
extern const char* PROGRAM_VERSION;

// Core application variables
extern Preferences myPreferences;
extern WebServer* webServer;

// Service status flags
extern bool wifiServiceStarted;
extern bool otaServiceStarted;
extern bool apModeActive;

// Sensor and output variables
extern uint64_t sensor0ReadMillis;
extern String sensor0Temperature;
extern String sensor0Humidity;
extern bool outputHeat;
extern bool outputFan;

// Initial WiFi scan variables
extern String initialWiFiScanResults;
extern bool initialScanComplete;

// Function declarations
void loadPreferences();
void connectWifi();
void startAPMode();
void performInitialWiFiScan();
void otaUpdatesInit();
