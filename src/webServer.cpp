#include "webServer.h"
#include <ArduinoOTA.h>

// Serving Home Page
void WebServer::getHomePage() {

  unsigned long currentMilliseconds = millis();

  String htmlPage;
  htmlPage.reserve(1024);               // prevent ram fragmentation
  htmlPage = F("<!DOCTYPE HTML>"
    "<html><head><meta http-equiv='refresh' content='5'></head>");

  htmlPage += "<h1>" + String(PROGRAM_NAME) + "&nbsp;" + String(PROGRAM_VERSION) + "</h1>";
  htmlPage += "<h2>[" + this->myPreferences->getString("device_name") + "]</h2>";

  htmlPage += "<form>";

  htmlPage += "<fieldset><legend>Status</legend>";
  htmlPage += "Sensor 0 - Temperature: " + String(sensor0Temperature) + " &deg;C<br/>";
  htmlPage += "Sensor 0 - Humidity: " + String(sensor0Humidity) + " %<br/>";
  htmlPage += "Heat: " + String(outputHeat ? "On" : "Off") + "<br/>";
  htmlPage += "Fan: " + String(outputFan ? "On" : "Off") + " <br/>";
  htmlPage += "</fieldset>";
  
  htmlPage += "<br/>";

  htmlPage += "<fieldset><legend>Settings</legend>";
  htmlPage += "</fieldset>";

  htmlPage += "<br/>";

  htmlPage += "<fieldset><legend>OTA Update</legend>";
  htmlPage += "OTA Service: " + String(otaServiceStarted ? "Enabled" : "Disabled") + "<br/>";
  htmlPage += "</fieldset>";

  htmlPage += "</form>";

  htmlPage += F("</html>"
    "\r\n");
      this->restServer->send(200, "text/html", htmlPage);
}

// Serving Status Data
void WebServer::getJsonStatus() {

  unsigned long currentMilliseconds = millis();

  JsonDocument doc;

  doc["device_name"] = this->myPreferences->getString("device_name");
  doc["wifi_signal_strengh"] = WiFi.RSSI();
  doc["board_free_heap"] = ESP.getFreeHeap();
  
  String buf;
  serializeJson(doc, buf);
  this->restServer->send(200, F("application/json"), buf);
}

// Serving Settings Data
void WebServer::getJsonSettings() {

  JsonDocument doc;
 
  // Load preferences
  for (JsonPair kv : objSettings) {

    const char* cKeyName = kv.key().c_str(); 
    const String keyType = kv.value()["type"].as<String>();

    if (keyType == "string") {
      if (kv.value()["obfuscate"])
        doc[cKeyName] = "*********";
      else 
        doc[cKeyName] = this->myPreferences->getString(cKeyName);
    } else if (keyType == "integer") {
      doc[cKeyName] = this->myPreferences->getInt(cKeyName);
    } else if (keyType == "boolean") {
      doc[cKeyName] = this->myPreferences->getBool(cKeyName);
    } else if (keyType == "float") {
      doc[cKeyName] = this->myPreferences->getFloat(cKeyName);
    }

  }

  if (this->restServer->arg("signalStrength") == "true"){
      doc["signalStrengh"] = WiFi.RSSI();
  }

  if (this->restServer->arg("chipInfo") == "true"){
      doc["chipId"] = ESP.getChipModel();
      doc["flashChipSize"] = ESP.getFlashChipSize();
  }

  if (this->restServer->arg("freeHeap") == "true"){
      doc["freeHeap"] = ESP.getFreeHeap();
  }
 
//      Serial.print(F("Stream..."));
  String buf;
  serializeJson(doc, buf);
  this->restServer->send(200, F("application/json"), buf);
//      Serial.print(F("done."));
}
 
// Set Settings
void WebServer::setSettings() {

  String html = "Attempting to update settings:<br/><br/>";

  for (uint8_t i = 0; i < this->restServer->args(); i++) {

    html += " <b>" + this->restServer->argName(i) + "</b> - ";

    String argName = this->restServer->argName(i);
    const char* cArgName = argName.c_str();     
    JsonDocument setting = this->objSettings[cArgName];

    // Find out if definition exists
    if (!this->objSettings[cArgName]) {
      html += "<i style='color:red;'>" + argName + " - Invalid setting</i><br/>";
      continue;
    }
    
    // Check if setting is editable
    if (setting["editable"] && setting["editable"].as<bool>() == false) {
      html += "<i style='color: red'> Non-editable setting " + argName + ".</i><br />";
      continue;
    }

    bool useDefault = (this->restServer->arg(i) == "");

    if (setting["type"].as<String>() == "string") {
        String newValue;
        html += "Current value: '" + this->myPreferences->getString(cArgName) + "'";
        if (useDefault) { newValue = setting["default"].as<String>(); }
        else { newValue = this->restServer->arg(i); }
        this->myPreferences->putString(cArgName, newValue);
        html += " - New value: '" + newValue + "'<br/>";
    }
    else if (setting["type"].as<String>() == "integer") {
        int newValue;
        html += "Current value: " + String(this->myPreferences->getInt(cArgName));
        if (useDefault) { newValue = this->objSettings[cArgName]["default"].as<unsigned int>(); }
        else { newValue = this->restServer->arg(i).toInt(); }
        this->myPreferences->putInt(cArgName, newValue);
        html += " - New value: " + String(newValue) + "<br/>";
    }
    else if (setting["type"].as<String>() == "bool") {
        bool newValue;
        html += "Current value: " + String(this->myPreferences->getBool(cArgName));
        if (useDefault) { newValue = this->objSettings[cArgName]["default"].as<bool>(); }
        else { newValue = this->restServer->arg(i).toInt(); }
        this->myPreferences->putInt(cArgName, newValue);
        html += " - New value: " + String(newValue) + "<br/>";
    }
    else if (setting["type"].as<String>() == "float") {
        float newValue;
        html += "Current value: " + String(this->myPreferences->getFloat(cArgName));
        if (useDefault) { newValue = this->objSettings[cArgName]["default"].as<float>(); }
        else { newValue = this->restServer->arg(i).toFloat(); }
        this->myPreferences->putFloat(cArgName, newValue);
        html += " - New value: " + String(newValue) + "<br/>";
    }    
    else {
      html += "<i style='color: red'> Unsupported type \"" + setting["type"].as<String>() + "\" for setting " + (String)cArgName + ".</i><br />";
    }
  }
  this->restServer->send(200, "text/html", html);
}

void WebServer::otaStart() {
  Serial.println("\nOTA Service Started");
  otaServiceStarted = true;
  ArduinoOTA.begin();
}

void WebServer::otaStop() {
  Serial.println("\nOTA Service Stopped");
  otaServiceStarted = false;
  ArduinoOTA.end();
}

// Manage not found URL
void WebServer::handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += this->restServer->uri();
  message += "\nMethod: ";
  message += (this->restServer->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += this->restServer->args();
  message += "\n";
  for (uint8_t i = 0; i < this->restServer->args(); i++) {
    message += " " + this->restServer->argName(i) + ": " + this->restServer->arg(i) + "\n";
  }
  this->restServer->send(404, "text/plain", message);
} 

void WebServer::espRestart() {
  ESP.restart();
}

void WebServer::initRoutes() {
    this->restServer->on("/", HTTP_GET, [this]() { this->getHomePage(); });
    this->restServer->on(F("/getJsonStatus"), HTTP_GET, [this]() { this->getJsonStatus(); });
    this->restServer->on(F("/getJsonSettings"), HTTP_GET, [this]() { this->getJsonSettings(); });
    this->restServer->on(F("/setSettings"), HTTP_GET, [this]() { this->setSettings(); });
    this->restServer->on(F("/otaStart"), HTTP_GET, [this]() { this->otaStart(); });
    this->restServer->on(F("/otaStop"), HTTP_GET, [this]() { this->otaStop(); });
    this->restServer->on(F("/espRestart"), HTTP_GET, [this]() { this->espRestart(); });

    // Set not found response
    this->restServer->onNotFound([this]() { this->handleNotFound(); });
    // Start server
    this->restServer->begin();
}

void WebServer::handleClient() {
  this->restServer->handleClient();
}

WebServer::WebServer(JsonObject objSettings, Preferences* myPreferences) {
  this->restServer = new ESP32WebServer(myPreferences->getInt("webserver_port"));
  this->objSettings = objSettings;
  this->myPreferences = myPreferences;

  this->initRoutes();
}

// Destructor
WebServer::~WebServer() {
  delete this->restServer;
}