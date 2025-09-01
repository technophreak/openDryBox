#include "webServer.h"
#include <ArduinoOTA.h>

// Serving Home Page
void WebServer::getHomePage() {

  unsigned long currentMilliseconds = millis();

  String htmlPage;
  htmlPage.reserve(2048);               // prevent ram fragmentation - increased for Bootstrap
  htmlPage = F("<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<meta http-equiv='refresh' content='5'>"
    "<title>openDryBox</title>"
    "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>"
    "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>"
    "</head>"
    "<body class='bg-light'>");

  // Header
  htmlPage += "<div class='container mt-4'>";
  htmlPage += "<div class='row'>";
  htmlPage += "<div class='col-12'>";
  htmlPage += "<h1 class='display-4 text-primary'>" + String(PROGRAM_NAME) + " <small class='text-muted'>" + String(PROGRAM_VERSION) + "</small></h1>";
  htmlPage += "<h2 class='h4 text-secondary mb-4'>[" + this->myPreferences->getString("device_name") + "]</h2>";
  htmlPage += "</div></div>";

  // Status Card
  htmlPage += "<div class='row mb-4'>";
  htmlPage += "<div class='col-md-6'>";
  htmlPage += "<div class='card'>";
  htmlPage += "<div class='card-header bg-primary text-white'>";
  htmlPage += "<h5 class='card-title mb-0'><i class='bi bi-thermometer'></i> Status</h5>";
  htmlPage += "</div>";
  htmlPage += "<div class='card-body'>";
  htmlPage += "<div class='row'>";
  htmlPage += "<div class='col-6'>";
  htmlPage += "<p class='mb-2'><strong>Temperature:</strong><br><span class='h5 text-info'>" + String(sensor0Temperature) + " °C</span></p>";
  htmlPage += "</div>";
  htmlPage += "<div class='col-6'>";
  htmlPage += "<p class='mb-2'><strong>Humidity:</strong><br><span class='h5 text-info'>" + String(sensor0Humidity) + " %</span></p>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "<hr>";
  htmlPage += "<div class='row'>";
  htmlPage += "<div class='col-6'>";
  htmlPage += "<p class='mb-1'><strong>Heat:</strong></p>";
  htmlPage += "<span class='badge " + String(outputHeat ? "bg-success" : "bg-secondary") + "'>" + String(outputHeat ? "On" : "Off") + "</span>";
  htmlPage += "</div>";
  htmlPage += "<div class='col-6'>";
  htmlPage += "<p class='mb-1'><strong>Fan:</strong></p>";
  htmlPage += "<span class='badge " + String(outputFan ? "bg-success" : "bg-secondary") + "'>" + String(outputFan ? "On" : "Off") + "</span>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";

  // Settings Card
  htmlPage += "<div class='col-md-6'>";
  htmlPage += "<div class='card'>";
  htmlPage += "<div class='card-header bg-secondary text-white'>";
  htmlPage += "<h5 class='card-title mb-0'><i class='bi bi-gear'></i> Settings</h5>";
  htmlPage += "</div>";
  htmlPage += "<div class='card-body'>";
  htmlPage += "<p class='text-muted'>Configure device settings and parameters.</p>";
  htmlPage += "<a href='/settings' class='btn btn-primary'>Open Settings</a>";
  htmlPage += "<button class='btn btn-outline-secondary ms-2' onclick='window.location.href=\"/getJsonSettings\"'>View JSON</button>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";

  // OTA Update Card
  htmlPage += "<div class='row mb-4'>";
  htmlPage += "<div class='col-12'>";
  htmlPage += "<div class='card'>";
  htmlPage += "<div class='card-header bg-warning text-dark'>";
  htmlPage += "<h5 class='card-title mb-0'><i class='bi bi-cloud-arrow-up'></i> OTA Update</h5>";
  htmlPage += "</div>";
  htmlPage += "<div class='card-body'>";
  htmlPage += "<p class='mb-2'><strong>OTA Service:</strong> ";
  htmlPage += "<span class='badge " + String(otaServiceStarted ? "bg-success" : "bg-danger") + "'>" + String(otaServiceStarted ? "Enabled" : "Disabled") + "</span></p>";
  htmlPage += "<div class='btn-group' role='group'>";
  htmlPage += "<button type='button' class='btn btn-outline-success btn-sm' onclick='window.location.href=\"/otaStart\"'>Start OTA</button>";
  htmlPage += "<button type='button' class='btn btn-outline-danger btn-sm' onclick='window.location.href=\"/otaStop\"'>Stop OTA</button>";
  htmlPage += "<button type='button' class='btn btn-outline-warning btn-sm' onclick='if(confirm(\"Are you sure you want to restart?\")) window.location.href=\"/espRestart\"'>Restart Device</button>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";

  htmlPage += "</div>"; // Close container

  htmlPage += F("</body></html>"
    "\r\n");
      this->restServer->send(200, "text/html", htmlPage);
}

// Serving Settings Page
void WebServer::getSettingsPage() {
  String htmlPage;
  htmlPage.reserve(3072);
  
  htmlPage = F("<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>Settings - openDryBox</title>"
    "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>"
    "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>"
    "<script>"
    "let originalValues = {};"
    "function trackOriginalValues() {"
    "  const form = document.getElementById('settingsForm');"
    "  const inputs = form.querySelectorAll('input');"
    "  inputs.forEach(input => {"
    "    if (input.type === 'checkbox') {"
    "      originalValues[input.name] = input.checked;"
    "    } else {"
    "      originalValues[input.name] = input.value;"
    "    }"
    "  });"
    "}"
    "function submitChangedOnly(event) {"
    "  event.preventDefault();"
    "  const form = document.getElementById('settingsForm');"
    "  const inputs = form.querySelectorAll('input');"
    "  const params = new URLSearchParams();"
    "  let hasChanges = false;"
    "  inputs.forEach(input => {"
    "    let currentValue;"
    "    if (input.type === 'checkbox') {"
    "      currentValue = input.checked;"
    "    } else {"
    "      currentValue = input.value;"
    "    }"
    "    if (currentValue !== originalValues[input.name]) {"
    "      if (input.type === 'checkbox') {"
    "        params.append(input.name, currentValue ? '1' : '0');"
    "      } else {"
    "        params.append(input.name, currentValue);"
    "      }"
    "      hasChanges = true;"
    "    }"
    "  });"
    "  if (hasChanges) {"
    "    window.location.href = '/setSettings?' + params.toString();"
    "  } else {"
    "    alert('No changes detected.');"
    "  }"
    "}"
    "function markAsChanged(element) {"
    "  element.classList.add('border-warning');"
    "  element.classList.remove('border-success');"
    "}"
    "function setupChangeTracking() {"
    "  const inputs = document.querySelectorAll('#settingsForm input');"
    "  inputs.forEach(input => {"
    "    input.addEventListener('input', () => markAsChanged(input));"
    "    input.addEventListener('change', () => markAsChanged(input));"
    "  });"
    "}"
    "</script>"
    "</head>"
    "<body class='bg-light' onload='trackOriginalValues(); setupChangeTracking();'>");

  // Header
  htmlPage += "<div class='container mt-4'>";
  htmlPage += "<div class='row'>";
  htmlPage += "<div class='col-12'>";
  htmlPage += "<h1 class='display-5 text-primary'>Settings <small class='text-muted'>Configuration</small></h1>";
  htmlPage += "<nav aria-label='breadcrumb'>";
  htmlPage += "<ol class='breadcrumb'>";
  htmlPage += "<li class='breadcrumb-item'><a href='/' class='text-decoration-none'>Home</a></li>";
  htmlPage += "<li class='breadcrumb-item active' aria-current='page'>Settings</li>";
  htmlPage += "</ol>";
  htmlPage += "</nav>";
  htmlPage += "</div></div>";

  // Settings Form
  htmlPage += "<form id='settingsForm' onsubmit='submitChangedOnly(event)'>";
  htmlPage += "<div class='row'>";
  htmlPage += "<div class='col-12'>";
  htmlPage += "<div class='card'>";
  htmlPage += "<div class='card-header bg-primary text-white'>";
  htmlPage += "<h5 class='card-title mb-0'><i class='bi bi-gear-fill'></i> Device Configuration</h5>";
  htmlPage += "</div>";
  htmlPage += "<div class='card-body'>";

  // Generate form fields based on settings
  for (JsonPair kv : objSettings) {
    const char* cKeyName = kv.key().c_str(); 
    const String keyType = kv.value()["type"].as<String>();
    const String keyLabel = kv.value()["label"] ? kv.value()["label"].as<String>() : String(cKeyName);
    const bool editable = !kv.value()["editable"] || kv.value()["editable"].as<bool>();
    
    if (!editable) continue;
    
    htmlPage += "<div class='mb-3'>";
    htmlPage += "<label for='" + String(cKeyName) + "' class='form-label'>" + keyLabel + "</label>";
    
    if (keyType == "string") {
      String currentValue = kv.value()["obfuscate"] ? "**********" : this->myPreferences->getString(cKeyName);
      String inputType = kv.value()["obfuscate"] ? "password" : "text";
      htmlPage += "<input type='" + inputType + "' class='form-control' id='" + String(cKeyName) + "' name='" + String(cKeyName) + "' value='" + currentValue + "'>";
    } else if (keyType == "integer") {
      int currentValue = this->myPreferences->getInt(cKeyName);
      htmlPage += "<input type='number' class='form-control' id='" + String(cKeyName) + "' name='" + String(cKeyName) + "' value='" + String(currentValue) + "'>";
    } else if (keyType == "boolean") {
      bool currentValue = this->myPreferences->getBool(cKeyName);
      htmlPage += "<div class='form-check'>";
      htmlPage += "<input class='form-check-input' type='checkbox' id='" + String(cKeyName) + "' name='" + String(cKeyName) + "' value='1'" + (currentValue ? " checked" : "") + ">";
      htmlPage += "<label class='form-check-label' for='" + String(cKeyName) + "'>" + keyLabel + "</label>";
      htmlPage += "</div>";
      htmlPage += "<input type='hidden' name='" + String(cKeyName) + "_hidden' value='0'>";  // Ensures unchecked boxes send a value
    } else if (keyType == "float") {
      float currentValue = this->myPreferences->getFloat(cKeyName);
      htmlPage += "<input type='number' step='0.01' class='form-control' id='" + String(cKeyName) + "' name='" + String(cKeyName) + "' value='" + String(currentValue) + "'>";
    }
    
    if (kv.value()["description"]) {
      htmlPage += "<div class='form-text'>" + kv.value()["description"].as<String>() + "</div>";
    }
    htmlPage += "</div>";
  }

  htmlPage += "<div class='d-grid gap-2 d-md-flex justify-content-md-end'>";
  htmlPage += "<button type='submit' class='btn btn-primary'>Save Changes</button>";
  htmlPage += "<button type='button' class='btn btn-outline-warning' onclick='location.reload()'>Reset Form</button>";
  htmlPage += "<a href='/' class='btn btn-outline-secondary'>Cancel</a>";
  htmlPage += "</div>";
  htmlPage += "<div class='mt-3'>";
  htmlPage += "<small class='text-muted'><i class='bi bi-info-circle'></i> Only changed values will be submitted. Modified fields will show a yellow border.</small>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</form>";

  htmlPage += "</div>"; // Close container
  htmlPage += F("</body></html>");
  
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

  String htmlPage;
  htmlPage.reserve(2048);
  
  htmlPage = F("<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>Settings Updated - openDryBox</title>"
    "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>"
    "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>"
    "</head>"
    "<body class='bg-light'>");

  htmlPage += "<div class='container mt-4'>";
  htmlPage += "<div class='row'>";
  htmlPage += "<div class='col-12'>";
  htmlPage += "<h1 class='display-5 text-primary'>Settings Updated</h1>";
  htmlPage += "<div class='card mt-4'>";
  htmlPage += "<div class='card-header bg-success text-white'>";
  htmlPage += "<h5 class='card-title mb-0'>Update Results</h5>";
  htmlPage += "</div>";
  htmlPage += "<div class='card-body'>";

  String results;
  int changedCount = 0;

  if (this->restServer->args() == 0) {
    results = "<div class='alert alert-info'><strong>No settings submitted.</strong> Use the settings form to make changes.</div>";
  } else {
    results = "<div class='alert alert-success'><strong>Settings Update Summary</strong><br/>The following " + String(this->restServer->args()) + " setting(s) were processed:</div>";

    for (uint8_t i = 0; i < this->restServer->args(); i++) {

      results += "<div class='mb-3 p-3 border rounded'><strong>" + this->restServer->argName(i) + "</strong><br/>";

      String argName = this->restServer->argName(i);
      const char* cArgName = argName.c_str();     
      JsonDocument setting = this->objSettings[cArgName];

      // Find out if definition exists
      if (!this->objSettings[cArgName]) {
        results += "<span class='badge bg-danger'>Invalid setting</span></div>";
        continue;
      }
      
      // Check if setting is editable
      if (setting["editable"] && setting["editable"].as<bool>() == false) {
        results += "<span class='badge bg-warning'>Non-editable setting</span></div>";
        continue;
      }

      bool useDefault = (this->restServer->arg(i) == "");

      if (setting["type"].as<String>() == "string") {
          String oldValue = this->myPreferences->getString(cArgName);
          String newValue;
          if (useDefault) { newValue = setting["default"].as<String>(); }
          else { newValue = this->restServer->arg(i); }
          
          this->myPreferences->putString(cArgName, newValue);
          results += "<small class='text-muted'>Previous: <code>" + oldValue + "</code></small><br/>";
          results += "<span class='text-success'>Updated to: <code>" + newValue + "</code></span>";
          if (oldValue != newValue) changedCount++;
      }
      else if (setting["type"].as<String>() == "integer") {
          int oldValue = this->myPreferences->getInt(cArgName);
          int newValue;
          if (useDefault) { newValue = this->objSettings[cArgName]["default"].as<unsigned int>(); }
          else { newValue = this->restServer->arg(i).toInt(); }
          
          this->myPreferences->putInt(cArgName, newValue);
          results += "<small class='text-muted'>Previous: <code>" + String(oldValue) + "</code></small><br/>";
          results += "<span class='text-success'>Updated to: <code>" + String(newValue) + "</code></span>";
          if (oldValue != newValue) changedCount++;
      }
      else if (setting["type"].as<String>() == "boolean") {
          bool oldValue = this->myPreferences->getBool(cArgName);
          bool newValue;
          if (useDefault) { newValue = this->objSettings[cArgName]["default"].as<bool>(); }
          else { newValue = this->restServer->arg(i).toInt(); }
          
          this->myPreferences->putBool(cArgName, newValue);
          results += "<small class='text-muted'>Previous: <code>" + String(oldValue ? "true" : "false") + "</code></small><br/>";
          results += "<span class='text-success'>Updated to: <code>" + String(newValue ? "true" : "false") + "</code></span>";
          if (oldValue != newValue) changedCount++;
      }
      else if (setting["type"].as<String>() == "float") {
          float oldValue = this->myPreferences->getFloat(cArgName);
          float newValue;
          if (useDefault) { newValue = this->objSettings[cArgName]["default"].as<float>(); }
          else { newValue = this->restServer->arg(i).toFloat(); }
          
          this->myPreferences->putFloat(cArgName, newValue);
          results += "<small class='text-muted'>Previous: <code>" + String(oldValue) + "</code></small><br/>";
          results += "<span class='text-success'>Updated to: <code>" + String(newValue) + "</code></span>";
          if (oldValue != newValue) changedCount++;
      }    
      else {
        results += "<span class='badge bg-danger'>Unsupported type: " + setting["type"].as<String>() + "</span>";
      }
      results += "</div>";
    }

    if (changedCount > 0) {
      results += "<div class='alert alert-warning mt-3'><strong>Note:</strong> " + String(changedCount) + " setting(s) were actually changed. Some settings may require a device restart to take effect.</div>";
    } else {
      results += "<div class='alert alert-info mt-3'><strong>No Changes:</strong> All submitted values were the same as current values.</div>";
    }
  }

  htmlPage += results;
  htmlPage += "<div class='mt-4'>";
  htmlPage += "<a href='/' class='btn btn-primary'>Return to Home</a>";
  htmlPage += "<a href='/settings' class='btn btn-outline-secondary ms-2'>Back to Settings</a>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += F("</body></html>");
  
  this->restServer->send(200, "text/html", htmlPage);
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
    this->restServer->on(F("/settings"), HTTP_GET, [this]() { this->getSettingsPage(); });
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