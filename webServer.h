#include <ESP32WebServer.h>

ESP32WebServer restServer(80);
 
// Serving Home Page
void getHomePage() {

  unsigned long currentMilliseconds = millis();

  String htmlPage;
  htmlPage.reserve(1024);               // prevent ram fragmentation
  htmlPage = F("<!DOCTYPE HTML>"
    "<html><head><meta http-equiv='refresh' content='5'></head>");

  htmlPage += "<h1>" + String(programName) + "&nbsp;" + String(programVersion) + "</h1>";
  htmlPage += "<h2>[" + myPreferences.getString("device_name") + "]</h2>";

  htmlPage += "<form>";

  htmlPage += "<fieldset><legend>Status</legend>";
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
      restServer.send(200, "text/html", htmlPage);
}

// Serving Status Data
void getJsonStatus() {

  unsigned long currentMilliseconds = millis();

  DynamicJsonDocument doc(512);

  doc["device_name"] = myPreferences.getString("device_name");
  doc["wifi_signal_strengh"] = WiFi.RSSI();
  doc["board_free_heap"] = ESP.getFreeHeap();
  
  String buf;
  serializeJson(doc, buf);
  restServer.send(200, F("application/json"), buf);
}

// Serving Settings Data
void getJsonSettings() {

      DynamicJsonDocument doc(512);
 
      doc["ip"] = WiFi.localIP().toString();
      doc["gw"] = WiFi.gatewayIP().toString();
      doc["nm"] = WiFi.subnetMask().toString();
 
      if (restServer.arg("signalStrength")== "true"){
          doc["signalStrengh"] = WiFi.RSSI();
      }
 
      if (restServer.arg("chipInfo")== "true"){
          doc["chipId"] = ESP.getChipModel();
          doc["flashChipSize"] = ESP.getFlashChipSize();
      }

      if (restServer.arg("freeHeap")== "true"){
          doc["freeHeap"] = ESP.getFreeHeap();
      }
 
//      Serial.print(F("Stream..."));
      String buf;
      serializeJson(doc, buf);
      restServer.send(200, F("application/json"), buf);
//      Serial.print(F("done."));
}
 
// Set Settings
void setSettings() {

  String html = "Attempting to update settings:<br/><br/>";

  for (uint8_t i = 0; i < restServer.args(); i++) {

    html += " <b>" + restServer.argName(i) + "</b> - ";

    // Find out if definition exists
    if ( !objSettings.containsKey(restServer.argName(i).c_str()) ) {
      html += "<i style='color:red;'>Invalid setting</i><br/>";
      continue;
    }
    
    bool useDefault = (restServer.arg(i) == "");

    if (objSettings[restServer.argName(i).c_str()]["type"].as<String>() == "string") {
        String newValue;
        html += "Current value: '" + myPreferences.getString(restServer.argName(i).c_str()) + "'";
        if (useDefault) { newValue = objSettings[restServer.argName(i).c_str()]["default"].as<String>(); }
        else { newValue = restServer.arg(i); }
        myPreferences.putString(restServer.argName(i).c_str(), newValue);
        html += " - New value: '" + newValue + "'<br/>";
    } else if (objSettings[restServer.argName(i).c_str()]["type"].as<String>() == "integer") {
        int newValue;
        html += "Current value: " + String(myPreferences.getInt(restServer.argName(i).c_str()));
        if (useDefault) { newValue = objSettings[restServer.argName(i).c_str()]["default"].as<unsigned int>(); }
        else { newValue = restServer.arg(i).toInt(); }
        myPreferences.putInt(restServer.argName(i).c_str(), newValue);
        html += " - New value: " + String(newValue) + "<br/>";
    } else if (objSettings[restServer.argName(i).c_str()]["type"].as<String>() == "bool") {
        int newValue;
        html += "Current value: " + String(myPreferences.getBool(restServer.argName(i).c_str()));
        if (useDefault) { newValue = objSettings[restServer.argName(i).c_str()]["default"].as<bool>(); }
        else { newValue = restServer.arg(i).toInt(); }
        myPreferences.putInt(restServer.argName(i).c_str(), newValue);
        html += " - New value: " + String(newValue) + "<br/>";
    } 
  }
  restServer.send(200, "text/html", html);
}

void otaStart() {
  Serial.println("\nOTA Service Started");
  otaServiceStarted = true;
  ArduinoOTA.begin();
}

void otaStop() {
  Serial.println("\nOTA Service Stopped");
  otaServiceStarted = false;
  ArduinoOTA.end();
}

// Manage not found URL
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += restServer.uri();
  message += "\nMethod: ";
  message += (restServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += restServer.args();
  message += "\n";
  for (uint8_t i = 0; i < restServer.args(); i++) {
    message += " " + restServer.argName(i) + ": " + restServer.arg(i) + "\n";
  }
  restServer.send(404, "text/plain", message);
}

void espRestart() {
  ESP.restart();
}

// Init Rest Server
void restServerInit() {

    restServer.on("/", HTTP_GET, getHomePage);
    restServer.on(F("/getJsonStatus"), HTTP_GET, getJsonStatus);
    restServer.on(F("/getJsonSettings"), HTTP_GET, getJsonSettings);
    restServer.on(F("/setSettings"), HTTP_GET, setSettings);
    restServer.on(F("/otaStart"), HTTP_GET, otaStart);
    restServer.on(F("/otaStop"), HTTP_GET, otaStop);
    restServer.on(F("/espRestart"), HTTP_GET, espRestart);

    // Set not found response
    restServer.onNotFound(handleNotFound);
    // Start server
    restServer.begin();
}