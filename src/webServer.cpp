#include "webServer.h"
#include "appFunctions.h"
#include <ArduinoOTA.h>

// Serving Home Page
void WebServer::getHomePage() {

  unsigned long currentMilliseconds = millis();

  String htmlPage;
  htmlPage.reserve(4096);               // prevent ram fragmentation - increased for Bootstrap and JavaScript
  htmlPage = F("<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>openDryBox</title>"
    "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>"
    "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>"
    "</head>");
  htmlPage += F("<body class='bg-light'>");

  // Toast container
  htmlPage += "<div id='toastContainer' class='toast-container position-fixed top-0 end-0 p-3' style='z-index: 1055;'></div>";

  // Header
  htmlPage += "<div class='container mt-4'>";
  htmlPage += "<div class='row'>";
  htmlPage += "<div class='col-12'>";
  htmlPage += "<h1 class='display-4 text-primary' id='deviceName'>Loading...</h1>";
  htmlPage += "<h2 class='h5 text-secondary mb-4'><span id='programName'>" + String(PROGRAM_NAME) + "</span> <small class='text-muted' id='programVersion'>" + String(PROGRAM_VERSION) + "</small></h2>";
  htmlPage += "</div></div>";

  // Top Row: Status and Presets
  htmlPage += "<div class='row mb-4'>";
  
  // Status Card
  htmlPage += "<div class='col-md-6 d-flex'>";
  htmlPage += "<div class='card h-100 w-100'>";
  htmlPage += "<div class='card-header bg-primary text-white'>";
  htmlPage += "<h5 class='card-title mb-0'><i class='bi bi-thermometer'></i> Status ";
  htmlPage += "<span id='connectionStatus' class='badge bg-warning text-dark' data-bs-toggle='tooltip' title='Connection pending...'>Pending</span>";
  htmlPage += "<small id='lastUpdate' class='text-white-50 ms-2'></small></h5>";
  htmlPage += "</div>";
  htmlPage += "<div class='card-body'>";
  htmlPage += "<div class='row'>";
  htmlPage += "<div class='col-6'>";
  htmlPage += "<p class='mb-2'><strong>Temperature:</strong><br><span id='temperature' class='h5 text-info'>--°C</span></p>";
  htmlPage += "</div>";
  htmlPage += "<div class='col-6'>";
  htmlPage += "<p class='mb-2'><strong>Humidity:</strong><br><span id='humidity' class='h5 text-info'>--%</span></p>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "<hr>";
  htmlPage += "<div class='row'>";
  htmlPage += "<div class='col-6'>";
  htmlPage += "<p class='mb-1'><strong>Heat:</strong></p>";
  htmlPage += "<span id='heatStatus' class='badge bg-secondary'>--</span>";
  htmlPage += "</div>";
  htmlPage += "<div class='col-6'>";
  htmlPage += "<p class='mb-1'><strong>Fan:</strong></p>";
  htmlPage += "<span id='fanStatus' class='badge bg-secondary'>--</span>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";

  // Presets Card
  htmlPage += "<div class='col-md-6 d-flex'>";
  htmlPage += "<div class='card h-100 w-100'>";
  htmlPage += "<div class='card-header bg-success text-white'>";
  htmlPage += "<h5 class='card-title mb-0'><i class='bi bi-bookmark'></i> Presets</h5>";
  htmlPage += "</div>";
  htmlPage += "<div class='card-body'>";
  htmlPage += "<p class='text-muted'>Quick access to predefined settings configurations.</p>";
  htmlPage += "<div class='text-center text-muted'>";
  htmlPage += "<i class='bi bi-plus-circle' style='font-size: 2rem;'></i>";
  htmlPage += "<p class='mt-2'>Coming Soon</p>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";

  // Bottom Row: Settings and OTA Service
  htmlPage += "<div class='row mb-4'>";
  
  // Settings Card
  htmlPage += "<div class='col-md-6 d-flex'>";
  htmlPage += "<div class='card h-100 w-100'>";
  htmlPage += "<div class='card-header bg-secondary text-white'>";
  htmlPage += "<h5 class='card-title mb-0'><i class='bi bi-gear'></i> Settings</h5>";
  htmlPage += "</div>";
  htmlPage += "<div class='card-body'>";
  htmlPage += "<p class='text-muted'>Configure device settings and parameters.</p>";
  htmlPage += "<a href='/settings' class='btn btn-primary'>Open Settings</a>";
  htmlPage += "<a href='/getJsonSettings' target='_blank' class='btn btn-outline-secondary ms-2'>View JSON</a>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";

  // OTA Service Card
  htmlPage += "<div class='col-md-6 d-flex'>";
  htmlPage += "<div class='card h-100 w-100'>";
  htmlPage += "<div class='card-header bg-warning text-dark'>";
  htmlPage += "<h5 class='card-title mb-0'><i class='bi bi-cloud-arrow-up'></i> OTA Service <span id='otaStatus' class='badge bg-secondary'>--</span></h5>";
  htmlPage += "</div>";
  htmlPage += "<div class='card-body'>";
  htmlPage += "<p class='text-muted'>Over-the-air firmware updates.</p>";
  htmlPage += "<div class='btn-group' role='group'>";
  htmlPage += "<button type='button' class='btn btn-outline-success btn-sm' onclick='otaAction(\"otaStart\")'>Start OTA</button>";
  htmlPage += "<button type='button' class='btn btn-outline-danger btn-sm' onclick='otaAction(\"otaStop\")'>Stop OTA</button>";
  htmlPage += "<button type='button' class='btn btn-outline-warning btn-sm' onclick='otaAction(\"espRestart\", \"Are you sure you want to restart the device?\")'>Restart Device</button>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";

  htmlPage += "</div>"; // Close container
  
  // Add complete JavaScript at the end
  htmlPage += "<script>";
  htmlPage += "let connectionState = 'pending';";
  htmlPage += "let reconnectInterval = 3000;";
  htmlPage += "let maxReconnectInterval = 30000;";
  htmlPage += "let fetchTimeout = " + String(this->myPreferences->getInt("ajax_timeout") * 1000) + ";"; // Convert seconds to milliseconds
  htmlPage += "let lastSuccessfulUpdate = null;";
  htmlPage += "let reconnectTimer = null;";
  htmlPage += "let currentVersion = '" + String(PROGRAM_VERSION) + "';"; // Track current version for reload detection
  htmlPage += "function fetchWithTimeout(url, options = {}) {";
  htmlPage += "  return Promise.race([";
  htmlPage += "    fetch(url, options),";
  htmlPage += "    new Promise((_, reject) => {";
  htmlPage += "      setTimeout(() => reject(new Error('Request timeout after ' + fetchTimeout + 'ms')), fetchTimeout);";
  htmlPage += "    })";
  htmlPage += "  ]);";
  htmlPage += "}";
  htmlPage += "function showToast(message, type = 'info') {";
  htmlPage += "  const toastContainer = document.getElementById('toastContainer');";
  htmlPage += "  const toastId = 'toast-' + Date.now();";
  htmlPage += "  const bgClass = type === 'success' ? 'bg-success' : type === 'error' ? 'bg-danger' : 'bg-info';";
  htmlPage += "  const toastHTML = '<div id=\"' + toastId + '\" class=\"toast ' + bgClass + ' text-white\" role=\"alert\">' + '<div class=\"toast-body\">' + message + '</div></div>';";
  htmlPage += "  toastContainer.insertAdjacentHTML('beforeend', toastHTML);";
  htmlPage += "  const toast = new bootstrap.Toast(document.getElementById(toastId));";
  htmlPage += "  toast.show();";
  htmlPage += "  setTimeout(() => document.getElementById(toastId).remove(), 5000);";
  htmlPage += "}";
  htmlPage += "function updateConnectionStatus(status, tooltip = '', showToastMsg = false) {";
  htmlPage += "  const badge = document.getElementById('connectionStatus');";
  htmlPage += "  if (!badge) return;";
  htmlPage += "  badge.textContent = status;";
  htmlPage += "  badge.setAttribute('data-bs-original-title', tooltip);";
  htmlPage += "  if (status === 'Connected') {";
  htmlPage += "    badge.className = 'badge bg-success';";
  htmlPage += "    if (connectionState === 'disconnected' && showToastMsg) {";
  htmlPage += "      showToast('Connection restored!', 'success');";
  htmlPage += "    }";
  htmlPage += "    connectionState = 'connected';";
  htmlPage += "    reconnectInterval = 3000;";
  htmlPage += "  } else if (status === 'Disconnected') {";
  htmlPage += "    badge.className = 'badge bg-danger';";
  htmlPage += "    if (connectionState === 'connected' && showToastMsg) {";
  htmlPage += "      showToast('Connection lost! Attempting to reconnect...', 'error');";
  htmlPage += "    }";
  htmlPage += "    connectionState = 'disconnected';";
  htmlPage += "  } else {";
  htmlPage += "    badge.className = 'badge bg-warning text-dark';";
  htmlPage += "    connectionState = 'pending';";
  htmlPage += "  }";
  htmlPage += "}";
  htmlPage += "function scheduleReconnect() {";
  htmlPage += "  if (reconnectTimer) clearTimeout(reconnectTimer);";
  htmlPage += "  reconnectTimer = setTimeout(() => {";
  htmlPage += "    updateStatus();";
  htmlPage += "    reconnectInterval = Math.min(reconnectInterval * 1.5, maxReconnectInterval);";
  htmlPage += "  }, reconnectInterval);";
  htmlPage += "}";
  htmlPage += "function otaAction(action, confirmMsg = null) {";
  htmlPage += "  if (confirmMsg && !confirm(confirmMsg)) return;";
  htmlPage += "  const btn = event.target;";
  htmlPage += "  const originalText = btn.textContent;";
  htmlPage += "  btn.disabled = true;";
  htmlPage += "  btn.textContent = 'Processing...';";
  htmlPage += "  fetchWithTimeout('/' + action).then(response => response.json()).then(data => {";
  htmlPage += "    showToast(data.message, data.status);";
  htmlPage += "    if (data.status === 'success' && action === 'espRestart') setTimeout(() => location.reload(), 3000);";
  htmlPage += "  }).catch(error => showToast('Error: ' + error.message, 'error')).finally(() => {";
  htmlPage += "    btn.disabled = false; btn.textContent = originalText;";
  htmlPage += "  });";
  htmlPage += "}";
  htmlPage += "function updateStatus() {";
  htmlPage += "  fetchWithTimeout('/getJsonStatus').then(response => {";
  htmlPage += "    if (!response.ok) throw new Error('HTTP ' + response.status);";
  htmlPage += "    return response.json();";
  htmlPage += "  }).then(data => {";
  htmlPage += "    const now = new Date();";
  htmlPage += "    lastSuccessfulUpdate = now;";
  htmlPage += "    updateConnectionStatus('Connected', 'Last update: ' + now.toLocaleString(), true);";
  htmlPage += "    const deviceNameElement = document.getElementById('deviceName');";
  htmlPage += "    if (deviceNameElement && data.device_name !== undefined) deviceNameElement.textContent = data.device_name;";
  htmlPage += "    const programNameElement = document.getElementById('programName');";
  htmlPage += "    if (programNameElement && data.program_name !== undefined) programNameElement.textContent = data.program_name;";
  htmlPage += "    const programVersionElement = document.getElementById('programVersion');";
  htmlPage += "    if (programVersionElement && data.program_version !== undefined) {";
  htmlPage += "      if (currentVersion !== data.program_version) {";
  htmlPage += "        document.getElementById('oldVersion').textContent = currentVersion;";
  htmlPage += "        document.getElementById('newVersion').textContent = data.program_version;";
  htmlPage += "        const versionModal = new bootstrap.Modal(document.getElementById('versionChangeModal'));";
  htmlPage += "        versionModal.show();";
  htmlPage += "        currentVersion = data.program_version;";
  htmlPage += "      }";
  htmlPage += "      programVersionElement.textContent = data.program_version;";
  htmlPage += "    }";
  htmlPage += "    const tempElement = document.getElementById('temperature');";
  htmlPage += "    if (tempElement && data.sensor0_temperature !== undefined) tempElement.textContent = data.sensor0_temperature + '°C';";
  htmlPage += "    const humidityElement = document.getElementById('humidity');";
  htmlPage += "    if (humidityElement && data.sensor0_humidity !== undefined) humidityElement.textContent = data.sensor0_humidity + '%';";
  htmlPage += "    const heatElement = document.getElementById('heatStatus');";
  htmlPage += "    if (heatElement && data.output_heat !== undefined) { heatElement.textContent = data.output_heat ? 'ON' : 'OFF'; heatElement.className = 'badge ' + (data.output_heat ? 'bg-danger' : 'bg-secondary'); }";
  htmlPage += "    const fanElement = document.getElementById('fanStatus');";
  htmlPage += "    if (fanElement && data.output_fan !== undefined) { fanElement.textContent = data.output_fan ? 'ON' : 'OFF'; fanElement.className = 'badge ' + (data.output_fan ? 'bg-primary' : 'bg-secondary'); }";
  htmlPage += "    const otaElement = document.getElementById('otaStatus');";
  htmlPage += "    if (otaElement && data.ota_service_started !== undefined) { otaElement.textContent = data.ota_service_started ? 'Active' : 'Inactive'; otaElement.className = 'badge ' + (data.ota_service_started ? 'bg-success' : 'bg-secondary'); }";
  htmlPage += "  }).catch(error => {";
  htmlPage += "    console.error('Update failed:', error);";
  htmlPage += "    const errorMsg = error.message.includes('Failed to fetch') ? 'Network error' : error.message;";
  htmlPage += "    updateConnectionStatus('Disconnected', 'Error: ' + errorMsg + ' at ' + new Date().toLocaleString(), true);";
  htmlPage += "    if (connectionState === 'disconnected') scheduleReconnect();";
  htmlPage += "  });";
  htmlPage += "}";
  htmlPage += "let updateInterval;";
  htmlPage += "function startRegularUpdates() {";
  htmlPage += "  if (updateInterval) clearInterval(updateInterval);";
  htmlPage += "  updateInterval = setInterval(() => {";
  htmlPage += "    if (connectionState !== 'disconnected') updateStatus();";
  htmlPage += "  }, 3000);";
  htmlPage += "}";
  htmlPage += "document.addEventListener('DOMContentLoaded', () => {";
  htmlPage += "  const tooltipTriggerList = [].slice.call(document.querySelectorAll('[data-bs-toggle=\"tooltip\"]'));";
  htmlPage += "  tooltipTriggerList.map(function (tooltipTriggerEl) { return new bootstrap.Tooltip(tooltipTriggerEl); });";
  htmlPage += "  updateStatus();";
  htmlPage += "  startRegularUpdates();";
  htmlPage += "});";
  htmlPage += "</script>";

  // Version Change Modal
  htmlPage += "<div class='modal fade' id='versionChangeModal' tabindex='-1' aria-labelledby='versionChangeModalLabel' aria-hidden='true'>";
  htmlPage += "<div class='modal-dialog modal-dialog-centered'>";
  htmlPage += "<div class='modal-content'>";
  htmlPage += "<div class='modal-header bg-warning text-dark'>";
  htmlPage += "<h5 class='modal-title' id='versionChangeModalLabel'><i class='bi bi-exclamation-triangle'></i> Application Updated</h5>";
  htmlPage += "<button type='button' class='btn-close' data-bs-dismiss='modal' aria-label='Close'></button>";
  htmlPage += "</div>";
  htmlPage += "<div class='modal-body'>";
  htmlPage += "<p class='mb-3'>The application version has changed:</p>";
  htmlPage += "<ul class='list-unstyled'>";
  htmlPage += "<li><strong>Previous:</strong> <span id='oldVersion' class='text-muted'></span></li>";
  htmlPage += "<li><strong>Current:</strong> <span id='newVersion' class='text-success'></span></li>";
  htmlPage += "</ul>";
  htmlPage += "<p class='mb-0'>It's recommended to reload the page to ensure you have the latest interface features.</p>";
  htmlPage += "</div>";
  htmlPage += "<div class='modal-footer'>";
  htmlPage += "<button type='button' class='btn btn-secondary' data-bs-dismiss='modal'>Later</button>";
  htmlPage += "<button type='button' class='btn btn-primary' onclick='location.reload()'>Reload Now</button>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";
  htmlPage += "</div>";

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