#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include <mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* programName = "openDryBox";
const char* programVersion = "v0.0.1";

const char* ota_password = "@rduin0";
bool otaServiceStarted = 0;

// Load Settings Definition
#include "allSettings.h"

void setup() {
  // Initialize Settings Definition
  initSettingsDefinition();  
}

void loop() {

}