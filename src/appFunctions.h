#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include "allSettings.h"

// External variables that need to be accessible
extern Preferences myPreferences;
extern bool wifiServiceStarted;

// Function declarations
void loadPreferences();
void connectWifi();
void otaUpdatesInit();
