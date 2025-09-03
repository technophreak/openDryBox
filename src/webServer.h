#ifndef _ODB_WEBSERVER_H_
#define _ODB_WEBSERVER_H_

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESP32WebServer.h>
#include <ESPmDNS.h>

#include "allSettings.h"
#include "appFunctions.h"

class WebServer {
  public:
    WebServer(JsonObject objSettings, Preferences* myPreferences);
    ~WebServer();
    void handleClient();
  private:
    void initRoutes();
    void getHomePage();
    void getSettingsPage();
    void getJsonStatus();
    void getJsonSettings();
    void getJsonFiles();
    void setSettings();
    void otaStart();
    void otaStop();
    void scanWiFiNetworks(); // Deprecated but still present
    void startWiFiScan();
    void getWiFiScanResults();
    void configureWiFi();
    void disableWiFi();
    void handleNotFound();
    void espRestart();
    void serveStaticFile(const String& path, const String& contentType);
    String getAllFilesJson(); // Get all files with sizes for debugging and progress calculation
    String processTemplate(String content); // Centralized template processing
    String getContentType(const String& path); // Determine MIME type from file extension
    void createDynamicStaticRoutes(); // Create routes based on filesystem scan

    ESP32WebServer* restServer;
    JsonObject objSettings;
    Preferences* myPreferences;
    
    // WiFi scan state tracking
    bool scanInProgress;
    bool scanComplete;
    JsonDocument scanResults;
    unsigned long scanStartTime;
};

#endif