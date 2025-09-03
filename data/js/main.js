// Application Main Functions
// This file contains the core application functionality after resource loading is complete

// Global variables and configuration
let connectionState = 'pending';
let reconnectInterval = 3000;
let maxReconnectInterval = 30000;
let lastSuccessfulUpdate = null;
let reconnectTimer = null;
let updateInterval;
let updateInProgress = false; // Prevent overlapping requests

let fetchTimeout = {{AJAX_TIMEOUT}};            // Will be replaced by server
let currentVersion = '{{PROGRAM_VERSION}}';     // Will be replaced by server
let currentProgramName = '{{PROGRAM_NAME}}';    // Will be replaced by server

// Application initialization function (called after all resources are loaded)
function initializeApplication() {
  console.log('Initializing application...');
  
  // Wait for Bootstrap to be fully ready
  setTimeout(function() {
    // Set up event listeners
    setupEventListeners();
    console.log('Application initialized successfully!');
  }, 1000);
}

// Utility function for fetch requests with timeout
function fetchWithTimeout(url, options = {}) {
  return Promise.race([
    fetch(url, options),
    new Promise((_, reject) => {
      setTimeout(() => reject(new Error('Request timeout after ' + fetchTimeout + 'ms')), fetchTimeout);
    })
  ]);
}

// Utility function for fetch requests with custom timeout (for WiFi scanning)
function fetchWithCustomTimeout(url, timeoutMs, options = {}) {
  return Promise.race([
    fetch(url, options),
    new Promise((_, reject) => {
      setTimeout(() => reject(new Error('Request timeout after ' + timeoutMs + 'ms')), timeoutMs);
    })
  ]);
}

// Toast notification system
function showToast(message, type = 'info') {
  const toastContainer = document.getElementById('toastContainer');
  const toastId = 'toast-' + Date.now();
  const bgClass = type === 'success' ? 'bg-success' : type === 'error' ? 'bg-danger' : 'bg-info';
  const toastHTML = '<div id="' + toastId + '" class="toast ' + bgClass + ' text-white" role="alert">' + 
                   '<div class="toast-body">' + message + '</div></div>';
  toastContainer.insertAdjacentHTML('beforeend', toastHTML);
  const toast = new bootstrap.Toast(document.getElementById(toastId));
  toast.show();
  setTimeout(() => document.getElementById(toastId).remove(), 5000);
}

// Connection status management
function updateConnectionStatus(status, tooltip = '', showToastMsg = false) {
  const badge = document.getElementById('connectionStatus');
  if (!badge) return;
  badge.textContent = status;
  badge.setAttribute('data-bs-original-title', tooltip);
  
  if (status === 'Connected') {
    badge.className = 'badge bg-success';
    if (connectionState === 'disconnected' && showToastMsg) {
      showToast('Connection restored!', 'success');
    }
    connectionState = 'connected';
    reconnectInterval = 3000;
  } else if (status === 'Disconnected') {
    badge.className = 'badge bg-danger';
    if (connectionState === 'connected' && showToastMsg) {
      showToast('Connection lost! Attempting to reconnect...', 'error');
    }
    connectionState = 'disconnected';
  } else {
    badge.className = 'badge bg-warning text-dark';
    connectionState = 'pending';
  }
}

// Reconnection scheduling
function scheduleReconnect() {
  if (reconnectTimer) clearTimeout(reconnectTimer);
  
  // Stop regular updates while in disconnected state
  if (updateInterval) {
    clearInterval(updateInterval);
    updateInterval = null;
  }
  
  reconnectTimer = setTimeout(() => {
    console.log('Attempting reconnection...');
    updateStatus();
    // Only schedule another reconnect if still disconnected
    if (connectionState === 'disconnected') {
      reconnectInterval = Math.min(reconnectInterval * 1.5, maxReconnectInterval);
      scheduleReconnect();
    } else {
      // If reconnected, restart regular updates
      startRegularUpdates();
      reconnectInterval = 3000; // Reset reconnect interval
    }
  }, reconnectInterval);
}

// OTA action handler
function otaAction(action, confirmMsg = null) {
  if (confirmMsg && !confirm(confirmMsg)) return;
  const btn = event.target;
  const originalText = btn.textContent;
  btn.disabled = true;
  btn.textContent = 'Processing...';
  
  fetchWithTimeout('/' + action)
    .then(response => response.json())
    .then(data => {
      showToast(data.message, data.status);
      if (data.status === 'success' && action === 'espRestart') {
        setTimeout(() => location.reload(), 3000);
      }
    })
    .catch(error => showToast('Error: ' + error.message, 'error'))
    .finally(() => {
      btn.disabled = false;
      btn.textContent = originalText;
    });
}

// Main status update function
function updateStatus() {
  // Prevent overlapping requests
  if (updateInProgress) {
    console.log('Update already in progress, skipping...');
    return;
  }
  
  updateInProgress = true;
  
  fetchWithTimeout('/getJsonStatus')
    .then(response => {
      if (!response.ok) throw new Error('HTTP ' + response.status);
      return response.json();
    })
    .then(data => {
      const now = new Date();
      lastSuccessfulUpdate = now;
      updateConnectionStatus('Connected', 'Last update: ' + now.toLocaleString(), true);
      
      // Update device information
      const deviceNameElement = document.getElementById('deviceName');
      if (deviceNameElement && data.device_name !== undefined) {
        deviceNameElement.textContent = data.device_name;
      }
      
      const programNameElement = document.getElementById('programName');
      if (programNameElement && data.program_name !== undefined) {
        programNameElement.textContent = data.program_name;
      }
      
      const programVersionElement = document.getElementById('programVersion');
      if (programVersionElement && data.program_version !== undefined) {
        if (currentVersion !== data.program_version) {
          document.getElementById('oldVersion').textContent = currentVersion;
          document.getElementById('newVersion').textContent = data.program_version;
          const versionModal = new bootstrap.Modal(document.getElementById('versionChangeModal'));
          versionModal.show();
          currentVersion = data.program_version;
        }
        programVersionElement.textContent = data.program_version;
      }
      
      // Update sensor data
      const tempElement = document.getElementById('temperature');
      if (tempElement && data.sensor0_temperature !== undefined) {
        tempElement.textContent = data.sensor0_temperature + '°C';
      }
      
      const humidityElement = document.getElementById('humidity');
      if (humidityElement && data.sensor0_humidity !== undefined) {
        humidityElement.textContent = data.sensor0_humidity + '%';
      }
      
      // Update output status
      const heatElement = document.getElementById('heatStatus');
      if (heatElement && data.output_heat !== undefined) {
        heatElement.textContent = data.output_heat ? 'ON' : 'OFF';
        heatElement.className = 'badge ' + (data.output_heat ? 'bg-danger' : 'bg-secondary');
      }
      
      const fanElement = document.getElementById('fanStatus');
      if (fanElement && data.output_fan !== undefined) {
        fanElement.textContent = data.output_fan ? 'ON' : 'OFF';
        fanElement.className = 'badge ' + (data.output_fan ? 'bg-primary' : 'bg-secondary');
      }
      
      const otaElement = document.getElementById('otaStatus');
      if (otaElement && data.ota_service_started !== undefined) {
        otaElement.textContent = data.ota_service_started ? 'Active' : 'Inactive';
        otaElement.className = 'badge ' + (data.ota_service_started ? 'bg-success' : 'bg-secondary');
      }
      
      updateWiFiConfigUI(data);
    })
    .catch(error => {
      console.error('Update failed:', error);
      const errorMsg = error.message.includes('Failed to fetch') ? 'Network error' : error.message;
      updateConnectionStatus('Disconnected', 'Error: ' + errorMsg + ' at ' + new Date().toLocaleString(), true);
      if (connectionState === 'disconnected') scheduleReconnect();
    })
    .finally(() => {
      updateInProgress = false; // Always clear the flag
    });
}

// Regular update scheduling
function startRegularUpdates() {
  if (updateInterval) clearInterval(updateInterval);
  if (reconnectTimer) {
    clearTimeout(reconnectTimer);
    reconnectTimer = null;
  }
  
  updateInterval = setInterval(() => {
    // Only run updates if not disconnected and no update is in progress
    if (connectionState !== 'disconnected' && !updateInProgress) {
      updateStatus();
    }
  }, 3000);
}

// WiFi configuration UI management
function updateWiFiConfigUI(data) {
  if (!data) return;
  
  const wifiConfigRow = document.getElementById('wifiConfigRow');
  const wifiMode = document.getElementById('wifiMode');
  const scanBtn = document.getElementById('scanWifiBtn');
  const disableBtn = document.getElementById('disableWifiBtn');
  const description = document.getElementById('wifiConfigDescription');
  
  // Hide WiFi config card entirely if WiFi is disabled in settings
  if (data.wifi_enabled === false) {
    wifiConfigRow.style.display = 'none';
    return;
  }
  
  if (data.ap_mode_active) {
    wifiConfigRow.style.display = 'flex';
    wifiMode.textContent = 'AP Mode';
    wifiMode.className = 'badge bg-warning text-dark';
    description.textContent = 'Device is in Access Point mode. Configure WiFi to connect to your network, or disable WiFi to stay in AP mode permanently.';
    scanBtn.style.display = 'block';
    disableBtn.style.display = 'block';
  } else if (!data.wifi_connected) {
    wifiConfigRow.style.display = 'flex';
    wifiMode.textContent = 'Disconnected';
    wifiMode.className = 'badge bg-danger';
    description.textContent = 'WiFi connection failed. You can reconfigure network settings or disable WiFi to stay in AP mode.';
    scanBtn.style.display = 'block';
    disableBtn.style.display = 'block';
  } else {
    wifiConfigRow.style.display = 'none';
  }
}

// Update WiFi configuration visibility
function updateWiFiConfigVisibility() {
  fetchWithTimeout('/getJsonStatus')
    .then(response => response.json())
    .then(data => {
      updateWiFiConfigUI(data);
    })
    .catch(() => document.getElementById('wifiConfigRow').style.display = 'none');
}

// WiFi scanning and modal display
function scanAndShowModal() {
  const modal = new bootstrap.Modal(document.getElementById('wifiModal'));
  const scanningMsg = document.getElementById('scanningMessage');
  const networkSelect = document.getElementById('networkSelect');
  
  // Show modal and load initial scan results
  networkSelect.innerHTML = '<option value="">Loading networks...</option>';
  modal.show();
  
  // Load initial scan results first
  loadInitialWiFiScan();
}

// Load initial WiFi scan results
function loadInitialWiFiScan() {
  const networkSelect = document.getElementById('networkSelect');
  
  fetchWithTimeout('/getWiFiScanResults')
    .then(response => response.json())
    .then(data => {
      networkSelect.innerHTML = '<option value="">Select a network...</option>';
      
      if (data.success && data.networks && data.networks.length > 0) {
        data.networks.forEach(network => {
          const option = document.createElement('option');
          option.value = network.ssid;
          option.textContent = network.ssid + ' (' + network.rssi + 'dBm, ' + network.encryption + ')';
          networkSelect.appendChild(option);
        });
        showToast('Loaded ' + data.networks.length + ' networks' + (data.message.includes('initial') ? ' from initial scan' : ''), 'success');
      } else {
        networkSelect.innerHTML = '<option value="">No networks found - try refresh</option>';
        showToast('No networks available - use refresh to scan', 'info');
      }
    })
    .catch(error => {
      networkSelect.innerHTML = '<option value="">Failed to load networks - try refresh</option>';
      showToast('Failed to load networks: ' + error.message, 'error');
    });
}

// Refresh WiFi networks (manual scan)
function refreshWiFiNetworks() {
  // IMMEDIATELY stop regular updates to prevent interference
  if (updateInterval) {
    clearInterval(updateInterval);
    updateInterval = null;
  }
  
  const scanningMsg = document.getElementById('scanningMessage');
  const networkSelect = document.getElementById('networkSelect');
  const refreshBtn = document.getElementById('refreshNetworksBtn');
  
  // Disable refresh button and show scanning state
  refreshBtn.disabled = true;
  refreshBtn.innerHTML = '⏳ Scanning...';
  scanningMsg.style.display = 'block';
  networkSelect.innerHTML = '<option value="">Starting scan...</option>';
  
  // Phase 1: Start the scan (quick response)
  fetchWithTimeout('/startWiFiScan')
    .then(response => response.json())
    .then(data => {
      if (data.success && data.scanning) {
        networkSelect.innerHTML = '<option value="">WiFi scan in progress...</option>';
        showToast('WiFi scan started', 'info');
        
        // Phase 2: Wait 10 seconds, then check for connection and poll for results
        setTimeout(() => {
          waitForConnectionAndPollResults();
        }, 10000); // Wait 10 seconds before first connection check
      } else {
        throw new Error(data.message || 'Failed to start scan');
      }
    })
    .catch(error => {
      scanningMsg.style.display = 'none';
      networkSelect.innerHTML = '<option value="">Failed to start scan</option>';
      showToast('Failed to start WiFi scan: ' + error.message, 'error');
      
      // Re-enable refresh button
      refreshBtn.disabled = false;
      refreshBtn.innerHTML = '🔄 Refresh';
      
      // Resume regular updates
      setTimeout(() => {
        startRegularUpdates();
      }, 2000);
    });
}
function waitForConnectionAndPollResults(maxWaitAttempts = 8, waitAttempt = 1, consecutiveSuccesses = 0) {
  // Check if there's already an update in progress, if so wait a bit
  if (updateInProgress) {
    console.log('Update in progress, waiting before connection check...');
    setTimeout(() => {
      waitForConnectionAndPollResults(maxWaitAttempts, waitAttempt, consecutiveSuccesses);
    }, 2000);
    return;
  }

  // Temporarily block other requests
  updateInProgress = true;
  
  // Try to verify connection is back by making a simple status call
  fetchWithTimeout('/getJsonStatus')
    .then(response => {
      if (response.ok) {
        consecutiveSuccesses++;
        console.log('Connection check successful (' + consecutiveSuccesses + '/2 required)');
        
        if (consecutiveSuccesses >= 2) {
          // Two consecutive successes - connection is stable, start polling for scan results
          console.log('Connection stable after', waitAttempt, 'attempts, starting result polling');
          const networkSelect = document.getElementById('networkSelect');
          networkSelect.innerHTML = '<option value="">Connection stable, checking scan progress...</option>';
          updateInProgress = false; // Clear the block
          pollForScanResults(20, 1, 0); // Start with no connection errors
        } else {
          // First success, wait a bit and check again
          updateInProgress = false; // Clear the block temporarily
          const networkSelect = document.getElementById('networkSelect');
          networkSelect.innerHTML = '<option value="">Connection check ' + consecutiveSuccesses + '/2 passed...</option>';
          
          setTimeout(() => {
            waitForConnectionAndPollResults(maxWaitAttempts, waitAttempt, consecutiveSuccesses);
          }, 2000); // Wait 2 seconds between consecutive checks
        }
      } else {
        throw new Error('Connection not ready');
      }
    })
    .catch(error => {
      updateInProgress = false; // Clear the block
      consecutiveSuccesses = 0; // Reset consecutive success counter
      
      if (waitAttempt < maxWaitAttempts) {
        // Connection not ready yet, wait longer between attempts
        setTimeout(() => {
          const networkSelect = document.getElementById('networkSelect');
          networkSelect.innerHTML = '<option value="">Waiting for connection... (' + waitAttempt + '/' + maxWaitAttempts + ')</option>';
          waitForConnectionAndPollResults(maxWaitAttempts, waitAttempt + 1, 0);
        }, 5000); // Wait 5 seconds between connection attempts
      } else {
        // Timeout waiting for connection
        const scanningMsg = document.getElementById('scanningMessage');
        const networkSelect = document.getElementById('networkSelect');
        
        scanningMsg.style.display = 'none';
        networkSelect.innerHTML = '<option value="">Connection timeout - try again</option>';
        showToast('Timeout waiting for connection restoration', 'error');
        
        // Resume regular updates
        setTimeout(() => {
          startRegularUpdates();
        }, 2000);
      }
    });
}

// Poll for WiFi scan results with connection error retry
function pollForScanResults(maxAttempts = 20, attempt = 1, connectionErrors = 0) {
  fetchWithCustomTimeout('/getWiFiScanResults', 5000) // Use custom timeout for scan results
    .then(response => response.json())
    .then(data => {
      connectionErrors = 0; // Reset connection error counter on success
      
      if (data.scanning) {
        // Still scanning, check again in 1 second
        if (attempt < maxAttempts) {
          setTimeout(() => {
            pollForScanResults(maxAttempts, attempt + 1, 0);
          }, 1000);
        } else {
          // Timeout
          throw new Error('Scan timeout after ' + maxAttempts + ' seconds');
        }
      } else {
        // Scan complete
        const scanningMsg = document.getElementById('scanningMessage');
        const networkSelect = document.getElementById('networkSelect');
        
        scanningMsg.style.display = 'none';
        networkSelect.innerHTML = '<option value="">Select a network...</option>';
        
        if (data.success && data.networks && data.networks.length > 0) {
          data.networks.forEach(network => {
            const option = document.createElement('option');
            option.value = network.ssid;
            option.textContent = network.ssid + ' (' + network.rssi + 'dBm, ' + network.encryption + ')';
            networkSelect.appendChild(option);
          });
          showToast('Found ' + data.networks.length + ' networks', 'success');
        } else {
          networkSelect.innerHTML = '<option value="">No networks found</option>';
          showToast(data.message || 'No WiFi networks found', 'info');
        }
        
        // Re-enable refresh button and hide scanning message
        const refreshBtn = document.getElementById('refreshNetworksBtn');
        const scanMsg = document.getElementById('scanningMessage');
        
        refreshBtn.disabled = false;
        refreshBtn.innerHTML = '🔄 Refresh';
        scanMsg.style.display = 'none';
        
        // Resume regular updates after scan completes
        setTimeout(() => {
          startRegularUpdates();
        }, 2000);
      }
    })
    .catch(error => {
      // Check if this is a connection error (common during WiFi mode switching)
      const isConnectionError = error.message.includes('fetch') || 
                               error.message.includes('timeout') || 
                               error.message.includes('NetworkError') ||
                               error.message.includes('Failed to fetch');
      
      if (isConnectionError && connectionErrors < 3 && attempt < maxAttempts) {
        // Connection error during scan - retry after a longer delay
        connectionErrors++;
        console.log('Connection error during scan polling (attempt ' + connectionErrors + '/3): ' + error.message);
        
        const networkSelect = document.getElementById('networkSelect');
        networkSelect.innerHTML = '<option value="">Connection interrupted, retrying... (' + connectionErrors + '/3)</option>';
        
        setTimeout(() => {
          pollForScanResults(maxAttempts, attempt + 1, connectionErrors);
        }, 3000); // Wait 3 seconds before retrying after connection error
      } else {
        // Permanent failure or too many connection errors
        const scanMsg2 = document.getElementById('scanningMessage');
        const networkSelect = document.getElementById('networkSelect');
        const refreshBtn = document.getElementById('refreshNetworksBtn');
        
        scanMsg2.style.display = 'none';
        refreshBtn.disabled = false;
        refreshBtn.innerHTML = '🔄 Refresh';
        
        if (connectionErrors >= 3) {
          networkSelect.innerHTML = '<option value="">Connection unstable - try again later</option>';
          showToast('WiFi scan failed: Connection unstable', 'error');
        } else {
          networkSelect.innerHTML = '<option value="">Scan failed - try again</option>';
          showToast('WiFi scan failed: ' + error.message, 'error');
        }
        
        // Resume regular updates
        setTimeout(() => {
          startRegularUpdates();
        }, 2000);
      }
      setTimeout(() => {
        startRegularUpdates();
      }, 2000);
    });
}

// Disable WiFi and stay in AP mode
function disableWiFiAndStayAP() {
  if (!confirm('Are you sure you want to disable WiFi and keep the device in AP mode permanently? You can re-enable WiFi later from the settings page.')) return;
  
  const btn = document.getElementById('disableWifiBtn');
  const originalText = btn.textContent;
  btn.disabled = true;
  btn.textContent = 'Disabling...';
  
  const formData = new FormData();
  formData.append('wifi_enabled', 'false');
  
  fetchWithTimeout('/disableWiFi', { method: 'POST', body: formData })
    .then(response => response.json())
    .then(data => {
      if (data.success) {
        showToast(data.message, 'success');
        setTimeout(() => location.reload(), 2000);
      } else {
        showToast(data.message, 'error');
      }
    })
    .catch(error => showToast('Failed to disable WiFi: ' + error.message, 'error'))
    .finally(() => {
      btn.disabled = false;
      btn.textContent = originalText;
    });
}

// DOM content loaded event handler
// Note: This will be called from initializeApplication() after all resources are loaded
function setupEventListeners() {
  // Initialize Bootstrap tooltips (only if Bootstrap is loaded)
  if (typeof bootstrap !== 'undefined') {
    const tooltipTriggerList = [].slice.call(document.querySelectorAll('[data-bs-toggle="tooltip"]'));
    tooltipTriggerList.map(function (tooltipTriggerEl) { 
      return new bootstrap.Tooltip(tooltipTriggerEl); 
    });
  }
  
  // WiFi configuration form submission
  const wifiForm = document.getElementById('wifiConfigForm');
  if (wifiForm) {
    wifiForm.addEventListener('submit', function(e) {
      e.preventDefault();
      const ssid = document.getElementById('networkSelect').value;
      const password = document.getElementById('wifiPassword').value;
      
      if (!ssid) { 
        showToast('Please select a network', 'error'); 
        return; 
      }
      
      const formData = new FormData();
      formData.append('ssid', ssid);
      formData.append('password', password);
      
      fetchWithTimeout('/configureWiFi', { method: 'POST', body: formData })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            showToast(data.message, 'success');
            if (typeof bootstrap !== 'undefined') {
              bootstrap.Modal.getInstance(document.getElementById('wifiModal')).hide();
            }
          } else {
            showToast(data.message, 'error');
          }
        })
        .catch(error => showToast('Configuration failed: ' + error.message, 'error'));
    });
  }

  // Settings form submission
  const settingsForm = document.getElementById('settingsForm');
  if (settingsForm) {
    settingsForm.addEventListener('submit', function(e) {
      e.preventDefault();
      saveSettings();
    });
  }
}

// Settings management functions
let originalSettingsValues = {};

// Open settings modal and load configuration
function openSettingsModal() {
  const modal = new bootstrap.Modal(document.getElementById('settingsModal'));
  modal.show();
  loadSettingsModal();
}

// Load settings configuration from server
function loadSettingsModal() {
  const loadingMsg = document.getElementById('settingsLoadingMessage');
  const settingsForm = document.getElementById('settingsForm');
  const settingsContent = document.getElementById('settingsContent');
  
  // Show loading, hide form
  loadingMsg.style.display = 'block';
  settingsForm.style.display = 'none';
  
  fetchWithTimeout('/getJsonConfig')
    .then(response => response.json())
    .then(data => {
      // Clear previous content
      settingsContent.innerHTML = '';
      originalSettingsValues = {};
      
      // Group settings by category
      const categories = {
        'Device': ['device_name'],
        'WiFi & Network': ['wifi_enabled', 'wifi_ssid', 'wifi_password', 'wifi_timeout', 'wifi_retries', 'network_dhcp', 'network_ip', 'network_subnet', 'network_gateway', 'network_dns_1', 'network_dns_2'],
        'Access Point': ['ap_ssid', 'ap_password'],
        'Web Server': ['webserver_port', 'ajax_timeout', 'ota_password'],
        'Sensor': ['sensor0_name', 'sensor0_pin', 'sensor0_type', 'sensor0_toffset', 'sensor0_hoffset'],
        'Hardware Control': ['mosfet_fan_pin', 'mosfet_ptc_pin'],
        'Temperature Control': ['ambEnabled', 'ambTempTarget']
      };
      
      // Generate form sections
      for (const [categoryName, categoryKeys] of Object.entries(categories)) {
        const categoryHtml = generateCategoryHtml(categoryName, categoryKeys, data);
        if (categoryHtml) {
          settingsContent.insertAdjacentHTML('beforeend', categoryHtml);
        }
      }
      
      // Track original values for change detection
      trackOriginalSettingsValues();
      
      // Setup change tracking
      setupSettingsChangeTracking();
      
      // Hide loading, show form
      loadingMsg.style.display = 'none';
      settingsForm.style.display = 'block';
      
      showToast('Settings loaded successfully', 'success');
    })
    .catch(error => {
      loadingMsg.style.display = 'none';
      settingsContent.innerHTML = '<div class="alert alert-danger">Failed to load settings: ' + error.message + '</div>';
      showToast('Failed to load settings: ' + error.message, 'error');
    });
}

// Generate HTML for a settings category
function generateCategoryHtml(categoryName, categoryKeys, settingsData) {
  let categoryHtml = '';
  let hasVisibleFields = false;
  
  // Filter keys that exist in the settings data and are editable
  const validKeys = categoryKeys.filter(key => {
    const setting = settingsData[key];
    return setting && (setting.editable !== false);
  });
  
  if (validKeys.length === 0) return '';
  
  categoryHtml += '<div class="card mb-3">';
  categoryHtml += '<div class="card-header bg-light">';
  categoryHtml += '<h6 class="card-title mb-0">' + categoryName + '</h6>';
  categoryHtml += '</div>';
  categoryHtml += '<div class="card-body">';
  
  validKeys.forEach(key => {
    const setting = settingsData[key];
    const fieldHtml = generateFieldHtml(key, setting);
    if (fieldHtml) {
      categoryHtml += fieldHtml;
      hasVisibleFields = true;
    }
  });
  
  categoryHtml += '</div>';
  categoryHtml += '</div>';
  
  return hasVisibleFields ? categoryHtml : '';
}

// Generate HTML for individual setting field
function generateFieldHtml(key, setting) {
  const type = setting.type;
  const label = setting.label || key;
  const description = setting.description || '';
  const value = setting.value;
  const isObfuscated = setting.obfuscate || false;
  
  let fieldHtml = '<div class="mb-3">';
  
  if (type === 'boolean') {
    fieldHtml += '<div class="form-check">';
    fieldHtml += '<input class="form-check-input settings-field" type="checkbox" id="' + key + '" name="' + key + '"' + (value ? ' checked' : '') + '>';
    fieldHtml += '<label class="form-check-label" for="' + key + '">' + label + '</label>';
    fieldHtml += '</div>';
  } else {
    fieldHtml += '<label for="' + key + '" class="form-label">' + label + '</label>';
    
    if (type === 'string') {
      const inputType = isObfuscated ? 'password' : 'text';
      const displayValue = isObfuscated ? '*********' : (value || '');
      fieldHtml += '<input type="' + inputType + '" class="form-control settings-field" id="' + key + '" name="' + key + '" value="' + displayValue + '">';
    } else if (type === 'integer') {
      fieldHtml += '<input type="number" class="form-control settings-field" id="' + key + '" name="' + key + '" value="' + (value || 0) + '">';
    } else if (type === 'float') {
      let step = '0.01';
      if (setting.maximum !== undefined && setting.maximum <= 100) step = '0.1';
      fieldHtml += '<input type="number" step="' + step + '" class="form-control settings-field" id="' + key + '" name="' + key + '" value="' + (value || 0) + '">';
    }
  }
  
  if (description) {
    fieldHtml += '<div class="form-text">' + description + '</div>';
  }
  
  fieldHtml += '</div>';
  
  return fieldHtml;
}

// Track original values for change detection
function trackOriginalSettingsValues() {
  const inputs = document.querySelectorAll('.settings-field');
  inputs.forEach(input => {
    if (input.type === 'checkbox') {
      originalSettingsValues[input.name] = input.checked;
    } else {
      originalSettingsValues[input.name] = input.value;
    }
  });
}

// Setup change tracking for settings fields
function setupSettingsChangeTracking() {
  const inputs = document.querySelectorAll('.settings-field');
  inputs.forEach(input => {
    input.addEventListener('input', () => markSettingAsChanged(input));
    input.addEventListener('change', () => markSettingAsChanged(input));
  });
}

// Mark setting field as changed
function markSettingAsChanged(element) {
  element.classList.add('border-warning');
  element.classList.remove('border-success');
}

// Save settings to server
function saveSettings() {
  const inputs = document.querySelectorAll('.settings-field');
  const changedSettings = {};
  let hasChanges = false;
  
  inputs.forEach(input => {
    let currentValue;
    if (input.type === 'checkbox') {
      currentValue = input.checked;
    } else if (input.type === 'number') {
      currentValue = parseFloat(input.value) || 0;
    } else {
      currentValue = input.value;
    }
    
    // Only include changed values (skip obfuscated fields showing asterisks)
    if (currentValue !== originalSettingsValues[input.name] && 
        !(input.type === 'password' && input.value === '*********')) {
      changedSettings[input.name] = currentValue;
      hasChanges = true;
    }
  });
  
  if (!hasChanges) {
    showToast('No changes detected', 'info');
    return;
  }
  
  // Disable form while saving
  const submitBtn = document.querySelector('#settingsForm button[type="submit"]');
  const originalText = submitBtn.textContent;
  submitBtn.disabled = true;
  submitBtn.textContent = 'Saving...';
  
  fetchWithTimeout('/setJsonSettings', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(changedSettings)
  })
    .then(response => response.json())
    .then(data => {
      if (data.success) {
        showToast(data.message, 'success');
        
        // Update original values to reflect saved state
        trackOriginalSettingsValues();
        
        // Remove warning borders from all fields
        const inputs = document.querySelectorAll('.settings-field');
        inputs.forEach(input => {
          input.classList.remove('border-warning');
          input.classList.add('border-success');
        });
        
        // Check if restart might be needed
        const criticalSettings = ['wifi_enabled', 'wifi_ssid', 'wifi_password', 'webserver_port'];
        const restartNeeded = Object.keys(changedSettings).some(key => criticalSettings.includes(key));
        
        if (restartNeeded && data.changedCount > 0) {
          showToast('Some changes may require a device restart to take effect', 'info');
        }
        
        // Close modal after short delay
        setTimeout(() => {
          if (typeof bootstrap !== 'undefined') {
            bootstrap.Modal.getInstance(document.getElementById('settingsModal')).hide();
          }
        }, 1500);
        
      } else {
        showToast(data.message || 'Failed to save settings', 'error');
      }
    })
    .catch(error => {
      showToast('Failed to save settings: ' + error.message, 'error');
    })
    .finally(() => {
      submitBtn.disabled = false;
      submitBtn.textContent = originalText;
    });
}

// Setup event listeners when DOM is ready  
document.addEventListener('DOMContentLoaded', () => {
  setupEventListeners();
});
