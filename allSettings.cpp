#include "allSettings.h"

const String allSettings = R"(
  {
    "device_name": { "type": "string", "default": "Untitled" },
    "wifi_enabled": { "type": "boolean", "default": true },
    "wifi_ssid": { "type": "string", "default": "" },
    "wifi_password": { "type": "string", "default": "", "obfuscate": true },
    "wifi_timeout": { "type": "integer", "default": 10 },
    "network_dhcp": { "type": "boolean", "default": true },
    "network_ip": { "type": "string", "default": "" },
    "network_subnet": { "type": "string", "default": "" },
    "wnetwork_gateway": { "type": "string", "default": "" },
    "network_dns_1": { "type": "string", "default": "" },
    "network_dns_2": { "type": "string", "default": "" },
    "webserver_port": { "type": "integer", "default": 80 },
  }
)";

DynamicJsonDocument jsonDoc(512);
JsonObject objSettings;

// Declare Preferences
Preferences myPreferences;

const char* ota_password = "@rduin0";
bool otaServiceStarted = 0;

bool wifiServiceStarted = false;

JsonObject initSettingsDefinition(String allSettings) {
  deserializeJson(jsonDoc, allSettings);
  return jsonDoc.as<JsonObject>();
}