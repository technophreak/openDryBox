#include "allSettings.h"

const String allSettings = R"(
  {
    "device_name": { "type": "string", "default": "Untitled", "label": "Device Name", "description": "Friendly name for this device" },
    
    "wifi_enabled": { "type": "boolean", "default": true, "label": "Enable WiFi", "description": "Enable or disable WiFi connectivity" },
    "wifi_ssid": { "type": "string", "default": "", "label": "WiFi Network Name", "description": "SSID of the WiFi network to connect to" },
    "wifi_password": { "type": "string", "default": "", "obfuscate": true, "label": "WiFi Password", "description": "Password for the WiFi network" },
    "wifi_timeout": { "type": "integer", "default": 10, "label": "WiFi Timeout", "description": "Timeout in seconds for WiFi connection attempts" },
    
    "network_dhcp": { "type": "boolean", "default": true, "label": "Use DHCP", "description": "Automatically obtain IP address from router" },
    "network_ip": { "type": "string", "default": "", "label": "Static IP Address", "description": "Manual IP address when DHCP is disabled" },
    "network_subnet": { "type": "string", "default": "", "label": "Subnet Mask", "description": "Network subnet mask for static IP configuration" },
    "network_gateway": { "type": "string", "default": "", "label": "Gateway Address", "description": "Router IP address for static IP configuration" },
    "network_dns_1": { "type": "string", "default": "", "label": "Primary DNS", "description": "Primary DNS server address" },
    "network_dns_2": { "type": "string", "default": "", "label": "Secondary DNS", "description": "Secondary DNS server address" },
    
    "webserver_port": { "type": "integer", "default": 80, "label": "Web Server Port", "description": "TCP port for the web interface" },
    "ajax_timeout": { "type": "integer", "default": 5, "label": "AJAX Timeout", "description": "Timeout in seconds for web interface API calls" },
    "ota_password": { "type": "string", "default": "pl@tformi0", "obfuscate": true, "label": "OTA Password", "description": "Password for over-the-air firmware updates" },
    
    "comments": "DHT22 Sensor",
    "sensor0_name": { "type": "string", "default": "Internal", "editable": false, "label": "Sensor Name", "description": "Display name for this sensor" },
    "sensor0_pin": { "type": "integer", "default": 5, "label": "Sensor Pin", "description": "GPIO pin connected to the DHT22 data line" },
    "sensor0_type": { "type": "string", "default": "DHT22", "label": "Sensor Type", "description": "Type of environmental sensor connected" },
    "sensor0_toffset": { "type": "float", "default": -3.5, "label": "Temperature Offset", "description": "Temperature calibration offset in degrees Celsius" },
    "sensor0_hoffset": { "type": "float", "default": 10, "label": "Humidity Offset", "description": "Humidity calibration offset in percentage" },

    "comments": "MOSFET Driver - Dual Parallel D4184",
    "mosfet_fan_pin": { "type": "integer", "default": 6, "label": "Fan Control Pin", "description": "GPIO pin for fan MOSFET control" },
    "mosfet_ptc_pin": { "type": "integer", "default": 7, "label": "PTC Heater Pin", "description": "GPIO pin for PTC heater MOSFET control" },

    "comments": "Ambient Temperature Control",
    "ambEnabled": { "type": "boolean", "default": false, "label": "Enable Temperature Control", "description": "Enable automatic ambient temperature control" },
    "ambTempTarget": { "type": "float", "default": 24, "maximum": 28, "label": "Target Temperature", "description": "Desired ambient temperature in degrees Celsius" },
    
  }
)";

JsonDocument jsonDoc;
JsonObject objSettings;

JsonObject initSettingsDefinition(String allSettings) {
  deserializeJson(jsonDoc, allSettings);
  return jsonDoc.as<JsonObject>();
}