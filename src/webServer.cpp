#include "webServer.h"
#include "appFunctions.h"
#include <ArduinoOTA.h>
#include <LittleFS.h>

// Serving Home Page
void WebServer::getHomePage() {
  // Try to load HTML from file
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    Serial.println("FILE: /index.html - NOT FOUND");
    // Fallback to serving a simple error page
    String errorPage = "<!DOCTYPE html><html><head><title>File Not Found</title></head>";
    errorPage += "<body><h1>Error</h1><p>HTML file not found. Please upload files to LittleFS.</p></body></html>";
    this->restServer->send(404, "text/html", errorPage);
    return;
  }

  size_t fileSize = file.size();
  String htmlContent = file.readString();
  file.close();
  
  Serial.print("FILE: /index.html - ");
  Serial.print(fileSize);
  Serial.println(" bytes");

  // Replace template placeholders with actual values
  String deviceName = this->myPreferences->getString("device_name");
  if (deviceName.isEmpty()) deviceName = "openDryBox";
  
  htmlContent.replace("{{DEVICE_NAME}}", deviceName);
  htmlContent.replace("{{PROGRAM_NAME}}", String(PROGRAM_NAME));
  htmlContent.replace("{{PROGRAM_VERSION}}", String(PROGRAM_VERSION));
  htmlContent.replace("{{AJAX_TIMEOUT}}", String(this->myPreferences->getInt("ajax_timeout") * 1000));

  this->restServer->send(200, "text/html", htmlContent);
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
    "<link href='/css/bootstrap.min.css' rel='stylesheet'>"
    "<script src='/js/bootstrap.bundle.min.js'></script>"
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
    
    // For non-boolean fields, add a label above the input
    if (keyType != "boolean") {
      htmlPage += "<label for='" + String(cKeyName) + "' class='form-label'>" + keyLabel + "</label>";
    }
    
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

  // Device information
  doc["device_name"] = this->myPreferences->getString("device_name");
  doc["program_name"] = PROGRAM_NAME;
  doc["program_version"] = PROGRAM_VERSION;
  doc["wifi_signal_strength"] = WiFi.RSSI();
  doc["board_free_heap"] = ESP.getFreeHeap();
  doc["uptime"] = currentMilliseconds;
  doc["ap_mode_active"] = apModeActive;
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  doc["wifi_enabled"] = this->myPreferences->getBool("wifi_enabled");
  
  // Sensor data
  doc["sensor0_temperature"] = sensor0Temperature;
  doc["sensor0_humidity"] = sensor0Humidity;
  
  // Output status
  doc["output_heat"] = outputHeat;
  doc["output_fan"] = outputFan;
  
  // OTA status
  doc["ota_service_started"] = otaServiceStarted;
  
  // Timestamp for client-side updates
  doc["timestamp"] = currentMilliseconds;
  
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

      String argName = this->restServer->argName(i);
      const char* cArgName = argName.c_str();     
      JsonDocument setting = this->objSettings[cArgName];
      
      // Get the user-friendly label, fallback to setting name if no label exists
      String displayName = setting["label"] ? setting["label"].as<String>() : argName;
      results += "<div class='mb-3 p-3 border rounded'><strong>" + displayName + "</strong>";
      if (setting["label"]) {
        results += "<br/><small class='text-muted'>(" + argName + ")</small>";
      }
      results += "<br/>";

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
  
  // Return JSON response for AJAX
  JsonDocument doc;
  doc["success"] = true;
  doc["message"] = "OTA service started successfully";
  doc["otaStatus"] = "enabled";
  
  String response;
  serializeJson(doc, response);
  this->restServer->send(200, "application/json", response);
}

void WebServer::otaStop() {
  Serial.println("\nOTA Service Stopped");
  otaServiceStarted = false;
  ArduinoOTA.end();
  
  // Return JSON response for AJAX
  JsonDocument doc;
  doc["success"] = true;
  doc["message"] = "OTA service stopped successfully";
  doc["otaStatus"] = "disabled";
  
  String response;
  serializeJson(doc, response);
  this->restServer->send(200, "application/json", response);
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
  // Return JSON response for AJAX first
  JsonDocument doc;
  doc["success"] = true;
  doc["message"] = "Device restart initiated";
  
  String response;
  serializeJson(doc, response);
  this->restServer->send(200, "application/json", response);
  
  // Add a small delay to ensure response is sent before restart
  delay(100);
  
  Serial.println("\nRestarting ESP32...");
  ESP.restart();
}

void WebServer::initRoutes() {
    Serial.println("Initializing web server routes...");
    
    this->restServer->on("/", HTTP_GET, [this]() { this->getHomePage(); });
    this->restServer->on(F("/settings"), HTTP_GET, [this]() { this->getSettingsPage(); });
    this->restServer->on(F("/getJsonStatus"), HTTP_GET, [this]() { this->getJsonStatus(); });
    this->restServer->on(F("/getJsonSettings"), HTTP_GET, [this]() { this->getJsonSettings(); });
    this->restServer->on(F("/setSettings"), HTTP_GET, [this]() { this->setSettings(); });
    this->restServer->on(F("/otaStart"), HTTP_GET, [this]() { this->otaStart(); });
    this->restServer->on(F("/otaStop"), HTTP_GET, [this]() { this->otaStop(); });
    this->restServer->on(F("/startWiFiScan"), HTTP_GET, [this]() { this->startWiFiScan(); });
    this->restServer->on(F("/getWiFiScanResults"), HTTP_GET, [this]() { this->getWiFiScanResults(); });
    this->restServer->on(F("/configureWiFi"), HTTP_POST, [this]() { this->configureWiFi(); });
    this->restServer->on(F("/disableWiFi"), HTTP_POST, [this]() { this->disableWiFi(); });
    this->restServer->on(F("/espRestart"), HTTP_GET, [this]() { this->espRestart(); });

    // Static file serving for /js/main.js
    this->restServer->on("/js/main.js", HTTP_GET, [this]() { this->serveStaticFile("/js/main.js", "text/javascript"); });
    
    // Static file serving - setup routes from array for easier maintenance
    Serial.println("Setting up static file routes...");
    
    struct StaticFileRoute {
        const char* path;
        const char* contentType;
        bool verbose; // Whether to log when route is called
    };
    
    StaticFileRoute staticFiles[] = {
        {"/css/bootstrap.min.css", "text/css", true},
        {"/css/bootstrap-icons.css", "text/css", true},
        {"/js/bootstrap.bundle.min.js", "text/javascript", true},
        {"/css/fonts/bootstrap-icons.woff2", "font/woff2", false},
        {"/css/fonts/bootstrap-icons.woff", "font/woff", false}
    };
    
    for (auto& route : staticFiles) {
        this->restServer->on(route.path, HTTP_GET, [this, route]() { 
            if (route.verbose) {
                Serial.print("Static file route called: ");
                Serial.println(route.path);
            }
            this->serveStaticFile(route.path, route.contentType); 
        });
    }

    // Set not found response
    this->restServer->onNotFound([this]() { this->handleNotFound(); });
    
    // Start server
    this->restServer->begin();
    Serial.println("Web server started successfully");
}

void WebServer::handleClient() {
  this->restServer->handleClient();
}

WebServer::WebServer(JsonObject objSettings, Preferences* myPreferences) {
  this->restServer = new ESP32WebServer(myPreferences->getInt("webserver_port"));
  this->objSettings = objSettings;
  this->myPreferences = myPreferences;
  
  // Initialize scan state
  this->scanInProgress = false;
  this->scanComplete = false;
  this->scanStartTime = 0;

  // Debug: List LittleFS files
  this->debugListLittleFSFiles();

  this->initRoutes();
}

// Start WiFi scan (immediate response, scan happens in background)
void WebServer::startWiFiScan() {
  Serial.println("Starting WiFi scan...");
  
  JsonDocument doc;
  
  if (scanInProgress) {
    doc["success"] = false;
    doc["message"] = "Scan already in progress";
    doc["scanning"] = true;
  } else {
    // Mark scan as started
    scanInProgress = true;
    scanComplete = false;
    scanStartTime = millis();
    
    doc["success"] = true;
    doc["message"] = "WiFi scan started";
    doc["scanning"] = true;
    
    // Send response immediately before starting scan
    String response;
    serializeJson(doc, response);
    this->restServer->send(200, "application/json", response);
    
    // Now start the actual scan in background
    Serial.println("Initiating background WiFi scan...");
    
    // Store current mode and AP configuration
    wifi_mode_t currentMode = WiFi.getMode();
    String currentAPSSID = "";
    String currentAPPassword = "";
    
    if (currentMode == WIFI_AP && apModeActive) {
      // Get current AP config to restore later
      currentAPSSID = this->myPreferences->getString("ap_ssid");
      currentAPPassword = this->myPreferences->getString("ap_password");
      
      // Switch to AP+STA mode
      WiFi.mode(WIFI_AP_STA);
      delay(500);
      
      // Restore AP configuration after mode switch
      if (currentAPPassword.length() > 0) {
        WiFi.softAP(currentAPSSID.c_str(), currentAPPassword.c_str());
      } else {
        WiFi.softAP(currentAPSSID.c_str(), "", 1, 0, 4);
      }
      delay(200);
    }
    
    // Perform the scan
    int networksFound = WiFi.scanNetworks(false, false, false, 200);
    
    // Process results
    scanResults.clear();
    
    if (networksFound == WIFI_SCAN_FAILED) {
      Serial.println("WiFi scan failed");
      scanResults["success"] = false;
      scanResults["message"] = "WiFi scan failed";
      scanResults["count"] = 0;
      scanResults["networks"] = JsonArray();
    } else if (networksFound == 0) {
      Serial.println("No networks found");
      scanResults["success"] = true;
      scanResults["message"] = "No networks found";
      scanResults["count"] = 0;
      scanResults["networks"] = JsonArray();
    } else {
      Serial.printf("Found %d networks\n", networksFound);
      
      JsonArray networks = scanResults["networks"].to<JsonArray>();
      
      for (int i = 0; i < networksFound; i++) {
        JsonObject network = networks.add<JsonObject>();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
        Serial.printf("  %d: %s (%d dBm) %s\n", i+1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured");
      }
      
      scanResults["success"] = true;
      scanResults["count"] = networksFound;
    }
    
    // Restore original mode if we were in pure AP mode
    if (currentMode == WIFI_AP && apModeActive) {
      Serial.println("Restoring AP-only mode...");
      WiFi.mode(WIFI_AP);
      delay(200);
      
      // Restore AP configuration
      if (currentAPPassword.length() > 0) {
        WiFi.softAP(currentAPSSID.c_str(), currentAPPassword.c_str());
      } else {
        WiFi.softAP(currentAPSSID.c_str(), "", 1, 0, 4);
      }
      delay(300);
      Serial.println("AP mode restored");
    }
    
    WiFi.scanDelete();
    scanInProgress = false;
    scanComplete = true;
    
    Serial.println("Background WiFi scan completed");
    return; // Response already sent
  }
  
  String response;
  serializeJson(doc, response);
  this->restServer->send(200, "application/json", response);
}

// Get WiFi scan results
void WebServer::getWiFiScanResults() {
  JsonDocument doc;
  
  if (scanInProgress) {
    doc["success"] = false;
    doc["message"] = "Scan still in progress";
    doc["scanning"] = true;
    doc["elapsed"] = millis() - scanStartTime;
  } else if (scanComplete) {
    // Return the stored results from on-demand scan
    doc = scanResults;
    doc["scanning"] = false;
    doc["elapsed"] = millis() - scanStartTime;
  } else if (initialScanComplete && initialWiFiScanResults.length() > 0) {
    // No on-demand scan available, return initial scan results
    DeserializationError error = deserializeJson(doc, initialWiFiScanResults);
    if (error) {
      doc["success"] = false;
      doc["message"] = "Error parsing initial scan results";
      doc["scanning"] = false;
    } else {
      doc["scanning"] = false;
      doc["message"] = doc["message"].as<String>() + " (initial scan)";
    }
  } else {
    doc["success"] = false;
    doc["message"] = "No scan results available";
    doc["scanning"] = false;
  }
  
  String response;
  serializeJson(doc, response);
  this->restServer->send(200, "application/json", response);
}

// WiFi network scanning - DEPRECATED: Use startWiFiScan/getWiFiScanResults instead
void WebServer::scanWiFiNetworks() {
  Serial.println("Scanning for WiFi networks...");
  
  // Store current mode and AP configuration
  wifi_mode_t currentMode = WiFi.getMode();
  String currentAPSSID = "";
  String currentAPPassword = "";
  
  if (currentMode == WIFI_AP && apModeActive) {
    // We're in AP mode, need to temporarily enable STA for scanning
    Serial.println("Preparing for scan while maintaining AP...");
    
    // Get current AP config to restore later
    currentAPSSID = this->myPreferences->getString("ap_ssid");
    currentAPPassword = this->myPreferences->getString("ap_password");
    
    // Switch to AP+STA mode more gracefully
    WiFi.mode(WIFI_AP_STA);
    delay(500); // Longer delay for stable mode switch
    
    // Restore AP configuration after mode switch
    if (currentAPPassword.length() > 0) {
      WiFi.softAP(currentAPSSID.c_str(), currentAPPassword.c_str());
    } else {
      WiFi.softAP(currentAPSSID.c_str(), "", 1, 0, 4);
    }
    delay(200); // Allow AP to stabilize
  }
  
  // Perform the scan with shorter channel time to reduce disruption
  int networksFound = WiFi.scanNetworks(false, false, false, 200); // Reduced from 300ms to 200ms per channel
  
  JsonDocument doc;
  
  if (networksFound == WIFI_SCAN_FAILED) {
    Serial.println("WiFi scan failed");
    doc["success"] = false;
    doc["message"] = "WiFi scan failed";
    doc["count"] = 0;
    doc["networks"] = JsonArray();
  } else if (networksFound == 0) {
    Serial.println("No networks found");
    doc["success"] = true;
    doc["message"] = "No networks found";
    doc["count"] = 0;
    doc["networks"] = JsonArray();
  } else {
    Serial.printf("Found %d networks\n", networksFound);
    
    JsonArray networks = doc["networks"].to<JsonArray>();
    
    for (int i = 0; i < networksFound; i++) {
      JsonObject network = networks.add<JsonObject>();
      network["ssid"] = WiFi.SSID(i);
      network["rssi"] = WiFi.RSSI(i);
      network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
      Serial.printf("  %d: %s (%d dBm) %s\n", i+1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured");
    }
    
    doc["success"] = true;
    doc["count"] = networksFound;
  }
  
  // Restore original mode if we were in pure AP mode
  if (currentMode == WIFI_AP && apModeActive) {
    Serial.println("Restoring AP-only mode...");
    WiFi.mode(WIFI_AP);
    delay(200);
    
    // Restore AP configuration
    if (currentAPPassword.length() > 0) {
      WiFi.softAP(currentAPSSID.c_str(), currentAPPassword.c_str());
    } else {
      WiFi.softAP(currentAPSSID.c_str(), "", 1, 0, 4);
    }
    delay(300); // Allow AP to fully stabilize
    Serial.println("AP mode restored");
  }
  
  String response;
  serializeJson(doc, response);
  this->restServer->send(200, "application/json", response);
  
  WiFi.scanDelete(); // Clean up scan results
}

// Configure WiFi settings
void WebServer::configureWiFi() {
  if (!this->restServer->hasArg("ssid") || !this->restServer->hasArg("password")) {
    JsonDocument doc;
    doc["success"] = false;
    doc["message"] = "Missing SSID or password";
    
    String response;
    serializeJson(doc, response);
    this->restServer->send(400, "application/json", response);
    return;
  }
  
  String newSSID = this->restServer->arg("ssid");
  String newPassword = this->restServer->arg("password");
  
  // Save new WiFi settings
  this->myPreferences->putString("wifi_ssid", newSSID);
  this->myPreferences->putString("wifi_password", newPassword);
  this->myPreferences->putBool("wifi_enabled", true);
  
  JsonDocument doc;
  doc["success"] = true;
  doc["message"] = "WiFi settings saved. Device will restart in 3 seconds to apply changes.";
  
  String response;
  serializeJson(doc, response);
  this->restServer->send(200, "application/json", response);
  
  // Schedule restart to apply new settings
  delay(100); // Give time for response to be sent
  ESP.restart();
}

// Disable WiFi and stay in AP mode
void WebServer::disableWiFi() {
  Serial.println("Disabling WiFi - staying in AP mode");
  
  // Save WiFi disabled setting
  this->myPreferences->putBool("wifi_enabled", false);
  
  JsonDocument doc;
  doc["success"] = true;
  doc["message"] = "WiFi disabled. Device will stay in AP mode permanently. You can re-enable WiFi from the settings page.";
  
  String response;
  serializeJson(doc, response);
  this->restServer->send(200, "application/json", response);
  
  Serial.println("WiFi disabled in settings");
}

// Serve static files from LittleFS with template processing
void WebServer::serveStaticFile(const String& path, const String& contentType) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.print("FILE: ");
    Serial.print(path);
    Serial.println(" - NOT FOUND");
    this->restServer->send(404, "text/plain", "File not found");
    return;
  }

  size_t fileSize = file.size();
  Serial.print("FILE: ");
  Serial.print(path);
  Serial.print(" - ");
  Serial.print(fileSize);
  Serial.println(" bytes");
  
  // Check if this file needs template processing
  bool needsTemplateProcessing = (path == "/js/main.js");
  
  if (needsTemplateProcessing) {
    Serial.println("Applying template processing...");
    
    // Read content for template processing
    String content = file.readString();
    file.close();
    
    // Apply template replacements
    content.replace("{{AJAX_TIMEOUT}}", String(this->myPreferences->getInt("ajax_timeout") * 1000));
    content.replace("{{PROGRAM_NAME}}", String(PROGRAM_NAME));
    content.replace("{{PROGRAM_VERSION}}", String(PROGRAM_VERSION));
    
    // Write processed content to temporary file
    String tempPath = path + ".tmp";
    File tempFile = LittleFS.open(tempPath, "w");
    if (!tempFile) {
      Serial.println("Failed to create temporary file for processing");
      this->restServer->send(500, "text/plain", "Processing error");
      return;
    }
    
    tempFile.print(content);
    tempFile.close();
    
    // Now stream the processed file
    File processedFile = LittleFS.open(tempPath, "r");
    if (!processedFile) {
      Serial.println("Failed to open processed temporary file");
      this->restServer->send(500, "text/plain", "Processing error");
      return;
    }
    
    this->restServer->streamFile(processedFile, contentType);
    processedFile.close();
    
    // Clean up temporary file
    LittleFS.remove(tempPath);
    Serial.println("Processed file streamed and cleaned up");
    
  } else {
    // For files that don't need processing, use streamFile directly
    Serial.println("Using direct streamFile (no processing needed)");
    this->restServer->streamFile(file, contentType);
    file.close();
    Serial.println("File streamed successfully");
  }
}

// Debug function to list LittleFS files
void WebServer::debugListLittleFSFiles() {
  Serial.println("=== LittleFS File Listing ===");
  File root = LittleFS.open("/");
  if (!root) {
    Serial.println("ERROR: Failed to open root directory");
    return;
  }
  
  if (!root.isDirectory()) {
    Serial.println("ERROR: Root is not a directory");
    return;
  }
  
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("DIR:  ");
      Serial.println(file.name());
      
      // List files in subdirectory
      File subfile = file.openNextFile();
      while (subfile) {
        Serial.print("  FILE: ");
        Serial.print(subfile.name());
        Serial.print(" (");
        Serial.print(subfile.size());
        Serial.println(" bytes)");
        subfile = file.openNextFile();
      }
    } else {
      Serial.print("FILE: ");
      Serial.print(file.name());
      Serial.print(" (");
      Serial.print(file.size());
      Serial.println(" bytes)");
    }
    file = root.openNextFile();
  }
  Serial.println("=== End File Listing ===");
}

// Destructor
WebServer::~WebServer() {
  delete this->restServer;
}