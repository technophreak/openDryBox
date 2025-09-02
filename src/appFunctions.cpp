#include "appFunctions.h"

void otaUpdatesInit()
{
  // OTA Updates
  ArduinoOTA.setPassword(myPreferences.getString("ota_password").c_str());
  ArduinoOTA.onStart([]() {
    Serial.println("\nOTA Request Started");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Request Ended");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("\nOTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
}

// Load preferences
void loadPreferences()
{
  // Init Namespace
  myPreferences.begin("settings", false); // Put true for read-only access

  // Remove all preferences under the opened namespace
  // REMOVE TO ALLOW CHANGING VALUES
  //myPreferences.clear();

  // Load preferences and set default if does not exist
  for (JsonPair kv : objSettings) {

    const char* keyName = kv.key().c_str(); 
    const String keyType = kv.value()["type"].as<String>();

    if (myPreferences.isKey(keyName) == false) {

      Serial.println("Initializing default '" + kv.value()["default"].as<String>() + "' value for " + keyName + " as type '" + keyType + "'");

      if (keyType == "string") {
        myPreferences.putString(keyName, kv.value()["default"].as<String>());
      } else if (keyType == "integer") {
        myPreferences.putInt(keyName, kv.value()["default"].as<unsigned int>());
      } else if (keyType == "boolean") {
        myPreferences.putBool(keyName, kv.value()["default"].as<bool>());
      } 

    } else { 
      if (keyType == "string") {
        if (kv.value()["obfuscate"])
          Serial.println("Preference for " + String(keyName) + " is set to '********'");
        else               
          Serial.println("Preference for " + String(keyName) + " is set to '" + String(myPreferences.getString(keyName)) + "'");        
      } else if (keyType == "integer") {
        Serial.println("Preference for " + String(keyName) + " is set to '" + String(myPreferences.getInt(keyName)) + "'");        
      } else if (keyType == "boolean") {
        Serial.println("Preference for " + String(keyName) + " is set to '" + String(myPreferences.getBool(keyName)) + "'");        
      }       
    }

  }

  // TEMP CODE - OVERRIDE CONFIG FOR DEBUGGING  
  //myPreferences.putBool("wifi_enabled", true); 
  //myPreferences.putInt("wifi_timeout", 10); 
  //myPreferences.putString("wifi_ssid", ""); 
  //myPreferences.putString("wifi_password", ""); 
  
}

// Connect to WiFi 
void connectWifi() 
{
  // Check if WiFi is enabled
  if (!myPreferences.getBool("wifi_enabled")) {
    Serial.println();
    Serial.println("WiFi disabled in settings - starting AP mode");
    startAPMode();
    return;
  }

  // Check if SSID is set
  if (myPreferences.getString("wifi_ssid") == "") {
    Serial.println();
    Serial.println("WiFi enabled but SSID is not defined - starting AP mode");
    startAPMode();
    return;
  }

  // Check if Password is set
  if (myPreferences.getString("wifi_password") == "") {
    Serial.println();
    Serial.println("WiFi enabled but Password is not defined - starting AP mode");
    startAPMode();
    return;
  }

  // Attempt WiFi connection with retries
  int maxRetries = myPreferences.getInt("wifi_retries");
  int timeoutSeconds = myPreferences.getInt("wifi_timeout");
  
  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    Serial.println();
    Serial.printf("WiFi connection attempt %d of %d", attempt, maxRetries);
    Serial.println();
    Serial.printf("Connecting to '%s' ", myPreferences.getString("wifi_ssid").c_str());

    // Init WiFi
    WiFi.begin(myPreferences.getString("wifi_ssid"), myPreferences.getString("wifi_password"));

    // Wait for connection
    int wifiDelay = 0;
    while (WiFi.status() != WL_CONNECTED && wifiDelay < (timeoutSeconds * 1000)) {
      delay(500);
      wifiDelay += 500;
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print("Connected, IP address: ");
      Serial.println(WiFi.localIP());
      Serial.println();
      wifiServiceStarted = true;
      apModeActive = false;
      return;
    } else {
      Serial.println();
      Serial.printf("Attempt %d failed", attempt);
      if (attempt < maxRetries) {
        Serial.println(" - retrying...");
        WiFi.disconnect();
        delay(2000); // Wait before next attempt
      }
    }
  }

  // All attempts failed, start AP mode
  Serial.println();
  Serial.println("All WiFi connection attempts failed - starting AP mode");
  startAPMode();
}

// Start Access Point mode
void startAPMode() 
{
  String apSSID = myPreferences.getString("ap_ssid");
  String apPassword = myPreferences.getString("ap_password");
  
  Serial.println();
  Serial.println("Starting Access Point mode...");
  
  // Perform initial WiFi scan before starting AP
  performInitialWiFiScan();
  
  Serial.printf("AP SSID: %s", apSSID.c_str());
  Serial.println();
  
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  delay(1000);
  
  bool apStarted = false;
  if (apPassword.length() > 0) {
    apStarted = WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    Serial.printf("AP Password: %s", apPassword.c_str());
  } else {
    // For open network, explicitly set no password
    apStarted = WiFi.softAP(apSSID.c_str(), "", 1, 0, 4); // channel 1, hidden 0, max connections 4
    Serial.println("AP Password: (open network)");
  }
  
  if (apStarted) {
    Serial.println();
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    Serial.println();
    
    wifiServiceStarted = true;
    apModeActive = true;
  } else {
    Serial.println();
    Serial.println("Failed to start AP mode!");
  }
}

// Perform initial WiFi scan for AP mode
void performInitialWiFiScan() 
{
  Serial.println("Performing initial WiFi scan...");
  
  // Set WiFi to station mode temporarily for scanning
  WiFi.mode(WIFI_STA);
  delay(100);
  
  // Perform the scan
  int networkCount = WiFi.scanNetworks();
  
  // Store results in the same format as on-demand scan would use
  // We'll set the global results that the WebServer can access
  if (networkCount == 0) {
    Serial.println("No networks found");
    initialWiFiScanResults = "{\"success\":true,\"message\":\"No networks found\",\"networks\":[],\"count\":0}";
  } else {
    Serial.printf("Found %d networks\n", networkCount);
    
    // Build JSON response in same format as getWiFiScanResults
    String json = "{\"success\":true,\"message\":\"Initial scan completed\",\"networks\":[";
    for (int i = 0; i < networkCount; i++) {
      if (i > 0) json += ",";
      
      String encryption = "Open";
      wifi_auth_mode_t authMode = WiFi.encryptionType(i);
      switch (authMode) {
        case WIFI_AUTH_WEP:
          encryption = "WEP";
          break;
        case WIFI_AUTH_WPA_PSK:
          encryption = "WPA";
          break;
        case WIFI_AUTH_WPA2_PSK:
          encryption = "WPA2";
          break;
        case WIFI_AUTH_WPA_WPA2_PSK:
          encryption = "WPA/WPA2";
          break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
          encryption = "WPA2-Enterprise";
          break;
        case WIFI_AUTH_WPA3_PSK:
          encryption = "WPA3";
          break;
        case WIFI_AUTH_WPA2_WPA3_PSK:
          encryption = "WPA2/WPA3";
          break;
        default:
          encryption = "Open";
          break;
      }
      
      json += "{";
      json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
      json += "\"encryption\":\"" + encryption + "\"";
      json += "}";
    }
    json += "],\"count\":" + String(networkCount) + "}";
    
    initialWiFiScanResults = json;
    Serial.println("Initial WiFi scan completed");
  }
  
  initialScanComplete = true;
  WiFi.scanDelete(); // Free scan results memory
}
