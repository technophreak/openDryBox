#pragma once
#ifndef _ALL_SETTINGS_H_
#define _ALL_SETTINGS_H_

#include <ArduinoJson.h>
#include <Preferences.h>

#define PROGRAM_NAME "openDryBox"
#define PROGRAM_VERSION "v0.0.4"

extern const String allSettings;
extern JsonObject objSettings;

extern bool otaServiceStarted;
extern bool wifiServiceStarted;

extern uint64_t sensor0ReadMillis;
extern String sensor0Temperature;
extern String sensor0Humidity;

extern bool outputHeat;
extern bool outputFan;

JsonObject initSettingsDefinition(String allSettings);

#endif