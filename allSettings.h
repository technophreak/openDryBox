#pragma once
#ifndef _ALL_SETTINGS_H_
#define _ALL_SETTINGS_H_

#include <ArduinoJson.h>
#include <Preferences.h>

#define PROGRAM_NAME "openDryBox"
#define PROGRAM_VERSION "v0.0.2"

extern const String allSettings;
extern JsonObject objSettings;

// Declare Preferences
extern Preferences myPreferences;

extern bool otaServiceStarted;
extern bool wifiServiceStarted;

JsonObject initSettingsDefinition(String allSettings);

#endif