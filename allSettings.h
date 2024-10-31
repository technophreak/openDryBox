#pragma once
#ifndef _ALL_SETTINGS_H_
#define _ALL_SETTINGS_H_

#include <ArduinoJson.h>
#include <Preferences.h>

#define PROGRAM_NAME "openDryBox"
#define PROGRAM_VERSION "v0.0.1"
#define WEBSERVER_PORT 80

extern const String allSettings;
extern JsonObject objSettings;

// Declare Preferences
extern Preferences myPreferences;

extern const char* ota_password;
extern bool otaServiceStarted;

extern bool wifiServiceStarted;

JsonObject initSettingsDefinition(String allSettings);

#endif