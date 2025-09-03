#include "webServer.h"
#include "appFunctions.h"
#include <ArduinoOTA.h>
#include <LittleFS.h>

// Determine content type based on file extension
String WebServer::getContentType(const String& path) {
  if (path.endsWith(".html") || path.endsWith(".htm")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js")) return "text/javascript";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
  if (path.endsWith(".gif")) return "image/gif";
  if (path.endsWith(".ico")) return "image/x-icon";
  if (path.endsWith(".svg")) return "image/svg+xml";
  if (path.endsWith(".woff")) return "font/woff";
  if (path.endsWith(".woff2")) return "font/woff2";
  if (path.endsWith(".ttf")) return "font/ttf";
  if (path.endsWith(".eot")) return "application/vnd.ms-fontobject";
  if (path.endsWith(".json")) return "application/json";
  if (path.endsWith(".xml")) return "text/xml";
  if (path.endsWith(".txt")) return "text/plain";
  return "application/octet-stream"; // Default binary type
}

// Create dynamic static file routes based on filesystem scan
void WebServer::createDynamicStaticRoutes() {
  Serial.println("Setting up dynamic static file routes...");
  
  // Use existing getAllFilesJson() function to get file list
  String filesJsonStr = this->getAllFilesJson();
  Serial.print("Files JSON: ");
  Serial.println(filesJsonStr);
  
  // Parse the JSON response
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, filesJsonStr);
  
  if (error) {
    Serial.print("Failed to parse files JSON: ");
    Serial.println(error.c_str());
    return;
  }
  
  JsonArray files = doc["files"];
  if (!files) {
    Serial.println("No files array found in JSON");
    return;
  }
  
  Serial.print("Found ");
  Serial.print(files.size());
  Serial.println(" files to create routes for");
  
  // Create routes for each file found
  int routeCount = 0;
  for (JsonObject file : files) {
    String filePath = file["path"].as<String>();
    String contentType = this->getContentType(filePath);
    
    Serial.print("Processing file: ");
    Serial.print(filePath);
    Serial.print(" -> ");
    Serial.println(contentType);
    
    // Determine if this file should have verbose logging
    bool verbose = (filePath.endsWith(".js") || filePath.endsWith(".css") || filePath.endsWith(".ico"));
    
    // Create the route using std::function to avoid memory issues
    this->restServer->on(filePath.c_str(), HTTP_GET, [this, filePath, contentType, verbose]() {
      if (verbose) {
        Serial.print("Static file route called: ");
        Serial.println(filePath);
      }
      this->serveStaticFile(filePath, contentType);
    });
    
    routeCount++;
    Serial.print("  Created route: ");
    Serial.print(filePath);
    Serial.print(" -> ");
    Serial.println(contentType);
  }
  
  Serial.print("Successfully created ");
  Serial.print(routeCount);
  Serial.println(" dynamic static file routes");
}

// Get all files with sizes - used for both debugging and progress calculation
String WebServer::getAllFilesJson() {
  JsonDocument doc;
  JsonArray files = doc["files"].to<JsonArray>();
  
  // Simple approach: scan root directory and subdirectories manually
  File root = LittleFS.open("/");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      String fileName = String(file.name());
      
      if (!file.isDirectory()) {
        JsonObject fileObj = files.add<JsonObject>();
        // Store full path for root files (add leading slash if not present)
        String fullPath = fileName.startsWith("/") ? fileName : "/" + fileName;
        fileObj["path"] = fullPath;
        fileObj["size"] = file.size();
        fileObj["type"] = "file";
      }
      file = root.openNextFile();
    }
    root.close();
  }
  
  // Check common subdirectories
  const char* subdirs[] = {"/css", "/js", "/css/fonts"};
  for (const char* subdir : subdirs) {
    File dir = LittleFS.open(subdir);
    if (dir && dir.isDirectory()) {
      File file = dir.openNextFile();
      while (file) {
        String fileName = String(file.name());
        
        if (!file.isDirectory()) {
          JsonObject fileObj = files.add<JsonObject>();
          // Store full path by combining subdir + filename
          String fullPath = String(subdir) + "/" + fileName;
          fileObj["path"] = fullPath;
          fileObj["size"] = file.size();
          fileObj["type"] = "file";
        }
        file = dir.openNextFile();
      }
      dir.close();
    }
  }
  
  // Add summary info
  doc["total_files"] = files.size();
  doc["timestamp"] = millis();
  
  // Use compact JSON format instead of pretty print
  String result;
  serializeJson(doc, result);
  return result;
}

// Centralized template replacement function
String WebServer::processTemplate(String content) {
  String deviceName = this->myPreferences->getString("device_name");
  if (deviceName.isEmpty()) deviceName = "openDryBox";
  
  content.replace("{{DEVICE_NAME}}", deviceName);
  content.replace("{{PROGRAM_NAME}}", String(PROGRAM_NAME));
  content.replace("{{PROGRAM_VERSION}}", String(PROGRAM_VERSION));
  content.replace("{{AJAX_TIMEOUT}}", String(this->myPreferences->getInt("ajax_timeout") * 1000));
  
  return content;
}

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

  // Process template replacements
  htmlContent = processTemplate(htmlContent);

  this->restServer->send(200, "text/html", htmlContent);
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

// Get a list of files
void WebServer::getJsonFiles() {
  String allFilesJson = getAllFilesJson();
  this->restServer->send(200, F("application/json"), allFilesJson);
}

// Get configuration schema from JSON file
void WebServer::getJsonConfig() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("CONFIG: /config.json - NOT FOUND");
    // Fallback to hardcoded settings if config.json is missing
    JsonDocument doc;
    
    // Load preferences using existing objSettings structure
    for (JsonPair kv : objSettings) {
      const char* cKeyName = kv.key().c_str(); 
      const String keyType = kv.value()["type"].as<String>();

      JsonObject configItem = doc[cKeyName].to<JsonObject>();
      configItem["type"] = kv.value()["type"];
      configItem["label"] = kv.value()["label"] ? kv.value()["label"].as<String>() : String(cKeyName);
      configItem["description"] = kv.value()["description"] ? kv.value()["description"].as<String>() : "";
      configItem["editable"] = !kv.value()["editable"] || kv.value()["editable"].as<bool>();
      if (kv.value()["obfuscate"]) configItem["obfuscate"] = kv.value()["obfuscate"].as<bool>();
      if (kv.value()["default"]) configItem["default"] = kv.value()["default"];
      if (kv.value()["maximum"]) configItem["maximum"] = kv.value()["maximum"];

      // Add current value
      if (keyType == "string") {
        if (kv.value()["obfuscate"]) {
          configItem["value"] = "*********";
        } else {
          configItem["value"] = this->myPreferences->getString(cKeyName);
        }
      } else if (keyType == "integer") {
        configItem["value"] = this->myPreferences->getInt(cKeyName);
      } else if (keyType == "boolean") {
        configItem["value"] = this->myPreferences->getBool(cKeyName);
      } else if (keyType == "float") {
        configItem["value"] = this->myPreferences->getFloat(cKeyName);
      }
    }
    
    String result;
    serializeJson(doc, result);
    this->restServer->send(200, F("application/json"), result);
    return;
  }

  size_t fileSize = configFile.size();
  String configContent = configFile.readString();
  configFile.close();
  
  Serial.print("CONFIG: /config.json - ");
  Serial.print(fileSize);
  Serial.println(" bytes");

  // Parse the JSON configuration
  JsonDocument configDoc;
  DeserializationError error = deserializeJson(configDoc, configContent);
  
  if (error) {
    Serial.print("Failed to parse config.json: ");
    Serial.println(error.c_str());
    this->restServer->send(500, F("application/json"), "{\"error\":\"Invalid configuration file\"}");
    return;
  }

  // Enhance the configuration with current values from preferences
  JsonDocument responseDoc;
  
  for (JsonPair kv : configDoc.as<JsonObject>()) {
    const char* cKeyName = kv.key().c_str(); 
    JsonObject configItem = kv.value().as<JsonObject>();
    const String keyType = configItem["type"].as<String>();
    
    JsonObject responseItem = responseDoc[cKeyName].to<JsonObject>();
    
    // Copy all configuration properties
    for (JsonPair configProp : configItem) {
      responseItem[configProp.key()] = configProp.value();
    }
    
    // Add current value from preferences
    if (keyType == "string") {
      if (configItem["obfuscate"] && configItem["obfuscate"].as<bool>()) {
        responseItem["value"] = "*********";
      } else {
        responseItem["value"] = this->myPreferences->getString(cKeyName);
      }
    } else if (keyType == "integer") {
      responseItem["value"] = this->myPreferences->getInt(cKeyName);
    } else if (keyType == "boolean") {
      responseItem["value"] = this->myPreferences->getBool(cKeyName);
    } else if (keyType == "float") {
      responseItem["value"] = this->myPreferences->getFloat(cKeyName);
    }
  }
  
  String result;
  serializeJson(responseDoc, result);
  this->restServer->send(200, F("application/json"), result);
}
 
// Set Settings via JSON POST
void WebServer::setJsonSettings() {
  if (this->restServer->method() != HTTP_POST) {
    this->restServer->send(405, F("application/json"), "{\"success\":false,\"message\":\"Method not allowed\"}");
    return;
  }

  String requestBody = this->restServer->arg("plain");
  if (requestBody.isEmpty()) {
    this->restServer->send(400, F("application/json"), "{\"success\":false,\"message\":\"Empty request body\"}");
    return;
  }

  JsonDocument requestDoc;
  DeserializationError error = deserializeJson(requestDoc, requestBody);
  
  if (error) {
    String errorResponse = "{\"success\":false,\"message\":\"Invalid JSON: " + String(error.c_str()) + "\"}";
    this->restServer->send(400, F("application/json"), errorResponse);
    return;
  }

  JsonDocument responseDoc;
  responseDoc["success"] = true;
  responseDoc["message"] = "Settings updated successfully";
  JsonArray changes = responseDoc["changes"].to<JsonArray>();
  
  int changedCount = 0;

  // Load config schema to validate settings
  JsonDocument configDoc;
  File configFile = LittleFS.open("/config.json", "r");
  bool useHardcodedConfig = false;
  
  if (configFile) {
    String configContent = configFile.readString();
    configFile.close();
    DeserializationError configError = deserializeJson(configDoc, configContent);
    if (configError) {
      useHardcodedConfig = true;
    }
  } else {
    useHardcodedConfig = true;
  }

  // Process each setting in the request
  for (JsonPair kv : requestDoc.as<JsonObject>()) {
    const char* cKeyName = kv.key().c_str();
    String keyName = String(cKeyName);
    
    JsonObject changeInfo = changes.add<JsonObject>();
    changeInfo["key"] = keyName;
    
    // Find setting definition (from config.json or fallback to objSettings)
    JsonObject settingDef;
    if (useHardcodedConfig && objSettings[cKeyName]) {
      settingDef = objSettings[cKeyName];
    } else if (!useHardcodedConfig && configDoc[cKeyName]) {
      settingDef = configDoc[cKeyName];
    } else {
      changeInfo["status"] = "error";
      changeInfo["message"] = "Unknown setting";
      continue;
    }
    
    // Check if setting is editable
    if (settingDef["editable"] && settingDef["editable"].as<bool>() == false) {
      changeInfo["status"] = "error";
      changeInfo["message"] = "Setting is not editable";
      continue;
    }
    
    String settingType = settingDef["type"].as<String>();
    String displayName = settingDef["label"] ? settingDef["label"].as<String>() : keyName;
    changeInfo["label"] = displayName;
    
    // Update the setting based on type
    if (settingType == "string") {
      String oldValue = this->myPreferences->getString(cKeyName);
      String newValue = kv.value().as<String>();
      
      this->myPreferences->putString(cKeyName, newValue);
      changeInfo["oldValue"] = oldValue;
      changeInfo["newValue"] = newValue;
      changeInfo["status"] = (oldValue != newValue) ? "changed" : "unchanged";
      if (oldValue != newValue) changedCount++;
      
    } else if (settingType == "integer") {
      int oldValue = this->myPreferences->getInt(cKeyName);
      int newValue = kv.value().as<int>();
      
      this->myPreferences->putInt(cKeyName, newValue);
      changeInfo["oldValue"] = oldValue;
      changeInfo["newValue"] = newValue;
      changeInfo["status"] = (oldValue != newValue) ? "changed" : "unchanged";
      if (oldValue != newValue) changedCount++;
      
    } else if (settingType == "boolean") {
      bool oldValue = this->myPreferences->getBool(cKeyName);
      bool newValue = kv.value().as<bool>();
      
      this->myPreferences->putBool(cKeyName, newValue);
      changeInfo["oldValue"] = oldValue;
      changeInfo["newValue"] = newValue;
      changeInfo["status"] = (oldValue != newValue) ? "changed" : "unchanged";
      if (oldValue != newValue) changedCount++;
      
    } else if (settingType == "float") {
      float oldValue = this->myPreferences->getFloat(cKeyName);
      float newValue = kv.value().as<float>();
      
      this->myPreferences->putFloat(cKeyName, newValue);
      changeInfo["oldValue"] = oldValue;
      changeInfo["newValue"] = newValue;
      changeInfo["status"] = (oldValue != newValue) ? "changed" : "unchanged";
      if (oldValue != newValue) changedCount++;
      
    } else {
      changeInfo["status"] = "error";
      changeInfo["message"] = "Unsupported setting type: " + settingType;
    }
  }

  responseDoc["changedCount"] = changedCount;
  
  if (changedCount > 0) {
    responseDoc["message"] = String(changedCount) + " setting(s) were updated";
  } else {
    responseDoc["message"] = "No settings were changed";
  }

  String response;
  serializeJson(responseDoc, response);
  this->restServer->send(200, F("application/json"), response);
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
    this->restServer->on(F("/getJsonStatus"), HTTP_GET, [this]() { this->getJsonStatus(); });
    this->restServer->on(F("/getJsonSettings"), HTTP_GET, [this]() { this->getJsonSettings(); });
    this->restServer->on(F("/getJsonConfig"), HTTP_GET, [this]() { this->getJsonConfig(); });
    this->restServer->on(F("/setJsonSettings"), HTTP_POST, [this]() { this->setJsonSettings(); });
    this->restServer->on(F("/getJsonFiles"), HTTP_GET, [this]() { this->getJsonFiles(); });
    this->restServer->on(F("/otaStart"), HTTP_GET, [this]() { this->otaStart(); });
    this->restServer->on(F("/otaStop"), HTTP_GET, [this]() { this->otaStop(); });
    this->restServer->on(F("/startWiFiScan"), HTTP_GET, [this]() { this->startWiFiScan(); });
    this->restServer->on(F("/getWiFiScanResults"), HTTP_GET, [this]() { this->getWiFiScanResults(); });
    this->restServer->on(F("/configureWiFi"), HTTP_POST, [this]() { this->configureWiFi(); });
    this->restServer->on(F("/disableWiFi"), HTTP_POST, [this]() { this->disableWiFi(); });
    this->restServer->on(F("/espRestart"), HTTP_GET, [this]() { this->espRestart(); });

    // Dynamic static file serving - automatically create routes based on filesystem
    this->createDynamicStaticRoutes();

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

  // Debug: Print all LittleFS files as JSON
  Serial.println("=== LittleFS Files (JSON Debug) ===");
  Serial.println(getAllFilesJson());
  Serial.println("=== End JSON Debug ===");

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
  bool needsTemplateProcessing = (path == "/js/main.js" || path == "/index.html");
  
  if (needsTemplateProcessing) {
    Serial.println("Applying template processing...");
    
    // Read content for template processing
    String content = file.readString();
    file.close();
    
    // Apply centralized template processing
    content = processTemplate(content);
    
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

// Destructor
WebServer::~WebServer() {
  delete this->restServer;
}