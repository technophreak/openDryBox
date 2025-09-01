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
    "ajax_timeout": { "type": "integer", "default": 10, "label": "AJAX Timeout in seconds", "description": "Timeout for web interface API calls"},
    "ota_password": { "type": "string", "default": "pl@tformi0", "obfuscate": true },
    
    "comments": "DHT22 Sensor",
    "sensor0_name": { "type": "string", "default": "Internal", "editable": false },
    "sensor0_pin": { "type": "integer", "default": 5 },
    "sensor0_type": { "type": "string", "default": "DHT22" },
    "sensor0_toffset": { "type": "float", "default": -3.5 },
    "sensor0_hoffset": { "type": "float", "default": 10 },

    "comments": "MOSFET Driver - Dual Parallel D4184",
    "mosfet_fan_pin": { "type": "integer", "default": 6 },
    "mosfet_ptc_pin": { "type": "integer", "default": 7 },

    "comments": "Ambient Temperature Control",
    "ambEnabled": { "type": "boolean", "default": false },
    "ambTempTarget": { "type": "float", "default": 24, "maximum": 28 },
    
  }
)";

JsonDocument jsonDoc;
JsonObject objSettings;

JsonObject initSettingsDefinition(String allSettings) {
  deserializeJson(jsonDoc, allSettings);
  return jsonDoc.as<JsonObject>();
}