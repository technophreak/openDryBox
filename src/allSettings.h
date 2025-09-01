#pragma once
#ifndef _ALL_SETTINGS_H_
#define _ALL_SETTINGS_H_

#include <ArduinoJson.h>
#include <Preferences.h>

extern const String allSettings;
extern JsonObject objSettings;

JsonObject initSettingsDefinition(String allSettings);

#endif