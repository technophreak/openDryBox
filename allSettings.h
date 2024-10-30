#include <ArduinoJson.h>

const String allSettings = R"(
  {
    "device_name": { "type": "string", "default": "Untitled" },
  }
)";

DynamicJsonDocument jsonDoc(512);
JsonObject objSettings;

void initSettingsDefinition() {
  deserializeJson(jsonDoc, allSettings);
  objSettings = jsonDoc.as<JsonObject>();
}
