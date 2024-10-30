#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

#include <ESP32WebServer.h>

// Serving Home Page
void getHomePage();
// Serving Status Data
void getJsonStatus();
// Serving Settings Data
void getJsonSettings();
 
// Set Settings
void setSettings();
void otaStart();
void otaStop();

// Manage not found URL
void handleNotFound();
void espRestart();

// Init Rest Server
void restServerInit();

#endif