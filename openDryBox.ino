#include <WiFi.h>
#include <WiFiClient.h>

#include <WiFiUdp.h>

// Load Settings Definition
#include "allSettings.h"
// Load Web Server
#include "webServer.h"

WebServer* webServer;

void setup() {

  // Init Serial 

  Serial.begin(921600);

  // Display program version
  Serial.println("\n\nProgram: " + String(PROGRAM_NAME) + " - " + String(PROGRAM_VERSION));

  // Initialize Settings Definition
  objSettings = initSettingsDefinition(allSettings);

  // Load Preferences
  loadPreferences();
 
  // Connect to Wifi
  connectWifi();

  if (wifiServiceStarted) {

    // Init Rest Server
    webServer = new WebServer(objSettings, &myPreferences, 80);

    // Init OTA Updates
    otaUpdatesInit();

  }

}

void loop() {

//  debugOutput();
  if (wifiServiceStarted) {
    webServer->handleClient();
    ArduinoOTA.handle();
  }
}

void otaUpdatesInit()
{

  // OTA Updates
  ArduinoOTA.setPassword(ota_password);
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



/* Load preferences */
void loadPreferences()
{
  // Init Namespace
  myPreferences.begin("settings", false); // Put true for read-only access

  // Remove all preferences under the opened namespace
  /* REMOVE TO ALLOW CHANGING VALUES */
  //myPreferences.clear();

  // Load preferences and set default if does not exist


  // TEMP CODE - OVERRIDE CONFIG FOR DEBUGGING
  if (false) {
    myPreferences.putBool("wifi_enabled", true); 
    myPreferences.putInt("wifi_timeout", 10); 
    myPreferences.putString("wifi_ssid", ""); 
    myPreferences.putString("wifi_password", ""); 
  }
  
}

/* Connect to WiFi */
void connectWifi() 
{

  // Check if WiFi is enabled
  if (!myPreferences.getBool("wifi_enabled")) {
    Serial.println();
    Serial.println("WiFi disabled in settings");
    return;
  }

  // Check if SSID is set
  if (myPreferences.getString("wifi_ssid") == "") {
    Serial.println();
    Serial.println("WiFi enabled but SSID is not defined");
    return;
  }

// Check if PAssword is set
  if (myPreferences.getString("wifi_password") == "") {
    Serial.println();
    Serial.println("WiFi enabled but Password is not defined");
    return;
  }

  // Init WiFi
  WiFi.begin(myPreferences.getString("wifi_ssid"), myPreferences.getString("wifi_password"));
  Serial.printf("Connecting to '%s' ", myPreferences.getString("wifi_ssid"));

  // Wait a bit for connection
  int wifiDelay = 0;
  while (WiFi.status() != WL_CONNECTED && wifiDelay < (myPreferences.getInt("wifi_timeout") * 1000))
  {
    delay(500);
    wifiDelay += 500;
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
    wifiServiceStarted = true;
  } else {
    Serial.println();
    Serial.println("Unable to connect");
  }

}