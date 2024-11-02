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
    "network_gateway": { "type": "string", "default": "" },
    "network_dns_1": { "type": "string", "default": "" },
    "network_dns_2": { "type": "string", "default": "" },
    "webserver_port": { "type": "integer", "default": 80 },
    "ota_password": { "type": "string", "default": "@rduin0", "obfuscate": true },
    
    // DHT22 Sensor on Liligo T-Display S3
    "sensor0_pin": { "type": "integer", "default": 16 },
    "sensor0_type": { "type": "string", "default": "DHT22" },
    "sensor0_toffset": { "type": "integer", "default": -3.5 },
    "sensor0_hoffset": { "type": "integer", "default": 10 },
    

  }
)";

DynamicJsonDocument jsonDoc(51200); // Max 50K
JsonObject objSettings;

// Declare Preferences
Preferences myPreferences;

bool otaServiceStarted = 0;

bool wifiServiceStarted = false;

JsonObject initSettingsDefinition(String allSettings) {
  deserializeJson(jsonDoc, allSettings);
  return jsonDoc.as<JsonObject>();
}