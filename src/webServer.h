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
    void setSettings();
    void otaStart();
    void otaStop();
    void handleNotFound();
    void espRestart();

    ESP32WebServer* restServer;
    JsonObject objSettings;
    Preferences* myPreferences;
};

#endif