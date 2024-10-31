#include <ArduinoJson.h>

const String allSettings = R"(
  {
    "device_name": { "type": "string", "default": "Untitled" },
    "wifi_enabled": { "type": "boolean", "default": true },
    "wifi_ssid": { "type": "string", "default": "" },
    "wifi_password": { "type": "string", "default": "" },
    "wifi_timeout": { "type": "integer", "default": 10 },
    "wifi_dhcp": { "type": "boolean", "default": true },
    "wifi_ip": { "type": "string", "default": "" },
    "wifi_subnet": { "type": "string", "default": "" },
    "wifi_gateway": { "type": "string", "default": "" },
    "wifi_dns_1": { "type": "string", "default": "" },
    "wifi_dns_2": { "type": "string", "default": "" },
  }
)";

DynamicJsonDocument jsonDoc(512);
JsonObject objSettings;

void initSettingsDefinition() {
  deserializeJson(jsonDoc, allSettings);
  objSettings = jsonDoc.as<JsonObject>();
}
