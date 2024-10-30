#include <ArduinoJson.h>

const String allSettings = R"(
  {
    "device_name": { "type": "string", "default": "Untitled" },
    "wifi_ssid": { "type": "string", "default": "" },
    "wifi_password": { "type": "string", "default": "" },
    "wifi_timeout": { "type": "int", "default": 10 },
    "wifi_dhcp": { "type": "boolean", "default": true },
  }
)";

DynamicJsonDocument jsonDoc(512);
JsonObject objSettings;

void initSettingsDefinition() {
  deserializeJson(jsonDoc, allSettings);
  objSettings = jsonDoc.as<JsonObject>();
}
