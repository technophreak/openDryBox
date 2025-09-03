// Application Initialization and Resource Loading System
// This file handles sequential resource loading and application startup for ESP32 constraints

// Sequential loading configuration - modify this array to change loading order
const RESOURCES_TO_LOAD = [
    { type: 'favicon', src: '/favicon.svg' },
    { type: 'js', src: '/js/main.js' },
    { type: 'css', src: '/css/bootstrap.min.css' },
    { type: 'js', src: '/js/bootstrap.bundle.min.js' },
    { type: 'css', src: '/css/bootstrap-icons.css' },
];

// Configuration constants
const CONFIG = {
    // Network detection settings
    DETECTION_TIMEOUT: 12000,    // 12 second maximum wait (ESP32 can be slow)
    CHECK_INTERVAL: 200,         // Check every 200ms (less frequent for ESP32)
    STABILITY_PERIOD: 1000,      // Wait 1 second of stability (longer for ESP32)
    FONT_TIMEOUT: 5000,          // 5 seconds specifically for font loading
    
    // Font loading settings
    MAX_FONT_WAIT: 8000,         // 8 seconds max wait for fonts before showing UI
    FONT_CHECK_INTERVAL: 200,    // Check font loading every 200ms
    
    // Loading settings
    MAX_RETRIES: 2,              // Maximum retry attempts
    RETRY_DELAY: 1000,           // Delay between retries (ms)
    RESOURCE_DELAY: 100,         // Delay between sequential resources (ms)
    UI_SHOW_DELAY: 300,          // Delay before showing UI (ms)
    FONT_CHECK_DELAY: 100,       // Delay before font loading check (ms)
    NETWORK_CHECK_DELAY: 300     // Initial delay for network monitoring (ms)
};

// Progress tracking variables for unified progress bar
let totalBytes = 0;
let completedBytes = 0;
let fileSizes = {};
let currentFileProgress = {};

// Cached DOM elements to avoid repeated lookups
let domCache = {};

// Initialize DOM cache
function initDOMCache() {
    domCache = {
        progressBar: document.getElementById('progressBar'),
        progressText: document.getElementById('progressText'),
        loadingStatus: document.getElementById('loadingStatus'),
        currentFile: document.getElementById('currentFile'),
        loadingOverlay: document.getElementById('loadingOverlay'),
        mainContainer: document.querySelector('.container.my-4')
    };
}

// Update unified progress based on total bytes loaded
function updateUnifiedProgress(filename, loaded, total, complete) {
    const fileSize = fileSizes[filename] || total;
    
    if (complete) {
        // File completed - add its full size to completedBytes
        completedBytes += fileSize;
        delete currentFileProgress[filename]; // Remove from current progress
    } else {
        // File in progress - track current progress
        currentFileProgress[filename] = loaded;
    }
    
    // Calculate total progress: completed files + current file progress
    let totalProgress = completedBytes;
    Object.values(currentFileProgress).forEach(progress => {
        totalProgress += progress;
    });
    
    // Calculate overall progress percentage
    const progressPercent = totalBytes > 0 ? Math.min(100, Math.round((totalProgress / totalBytes) * 100)) : 0;
    
    // Update progress bar using cached DOM elements
    if (domCache.progressBar) domCache.progressBar.style.width = progressPercent + '%';
    if (domCache.progressText) domCache.progressText.textContent = progressPercent + '%';
    
    // Update status messages
    updateProgressStatus(filename, loaded, fileSize, complete);
}

// Separate function for status message updates
function updateProgressStatus(filename, loaded, fileSize, complete) {
    if (filename.includes('Complete')) {
        if (domCache.loadingStatus) domCache.loadingStatus.textContent = 'Loading complete!';
        if (domCache.currentFile) domCache.currentFile.textContent = 'Initializing ...';
    } else if (complete) {
        if (domCache.loadingStatus) domCache.loadingStatus.textContent = 'Loading resources ...';
        if (domCache.currentFile) domCache.currentFile.textContent = `${filename} loaded`;
    } else {
        if (domCache.loadingStatus) domCache.loadingStatus.textContent = `Loading ${filename} ...`;
        if (domCache.currentFile) {
            const filePercent = fileSize > 0 ? Math.round((loaded / fileSize) * 100) : 0;
            domCache.currentFile.textContent = `${filename} (${filePercent}%)`;
        }
    }
}

// Handle resource loading errors with unified retry logic
function handleResourceError(resource, filename, retryCount, loadNextCallback) {
    if (retryCount < CONFIG.MAX_RETRIES) {
        console.log(`Will retry ${resource.type} for: ${filename} (attempt ${retryCount + 1}/${CONFIG.MAX_RETRIES + 1})`);
        setTimeout(() => loadNextCallback(retryCount + 1), CONFIG.RETRY_DELAY);
    } else {
        console.error(`Giving up on ${resource.type} after ${CONFIG.MAX_RETRIES} retries: ${filename}`);
        if (resource.type === 'js') {
            // For failed JS, we might want to update progress as complete to avoid hanging
            updateUnifiedProgress(filename, 0, fileSizes[filename] || 0, true);
        }
        setTimeout(() => loadNextCallback(0, true), CONFIG.RESOURCE_DELAY); // Move to next resource
    }
}

// Create DOM element for resource injection
function createResourceElement(resource, onLoad, onError) {
    let element;
    
    if (resource.type === 'css') {
        element = document.createElement('link');
        element.rel = 'stylesheet';
        element.type = 'text/css';
        element.href = resource.src;
        element.onload = onLoad;
        element.onerror = onError;
    } else if (resource.type === 'js') {
        element = document.createElement('script');
        element.type = 'text/javascript';
        element.onload = onLoad;
        element.onerror = onError;
    } else if (resource.type === 'favicon') {
        // Replace the existing placeholder favicon instead of adding a new one
        const existingFavicon = document.getElementById('favicon-placeholder') || document.querySelector('link[rel="icon"]');
        if (existingFavicon) {
            existingFavicon.href = resource.src;
            // Set type based on file extension
            if (resource.src.endsWith('.svg')) {
                existingFavicon.type = 'image/svg+xml';
            } else {
                existingFavicon.type = 'image/x-icon';
            }
            // Favicon onload events are unreliable, so we'll call onLoad immediately
            // after a short delay to allow the browser to process the change
            setTimeout(() => {
                onLoad();
            }, 100);
            existingFavicon.onerror = onError;
            return existingFavicon;
        } else {
            // Fallback: create new element if placeholder not found
            element = document.createElement('link');
            element.rel = 'icon';
            if (resource.src.endsWith('.svg')) {
                element.type = 'image/svg+xml';
            } else {
                element.type = 'image/x-icon';
            }
            element.href = resource.src;
            // Same issue with new favicon elements - onload is unreliable
            setTimeout(() => {
                onLoad();
            }, 100);
            element.onerror = onError;
        }
    }
    
    return element;
}
// Sequential resource loader with unified progress tracking and retry logic
function loadResourceSequentially(resources, index = 0, retryCount = 0) {
    if (index >= resources.length) {
        console.log('All resources loaded successfully');
        updateUnifiedProgress('Complete', totalBytes, totalBytes, true);
        initializeAfterResourcesReady();
        return;
    }
    
    const resource = resources[index];
    const filename = resource.src.split('/').pop();
    
    if (retryCount === 0) {
        console.log(`Starting: ${filename}`);
    } else {
        console.log(`Retrying: ${filename} (attempt ${retryCount + 1}/${CONFIG.MAX_RETRIES + 1})`);
    }
    
    // Use XMLHttpRequest for progress tracking
    const xhr = new XMLHttpRequest();
    xhr.open('GET', resource.src, true);
    
    // Track download progress
    xhr.onprogress = function(event) {
        if (event.lengthComputable) {
            updateUnifiedProgress(filename, event.loaded, event.total, false);
        }
    };
    
    xhr.onload = function() {
        if (xhr.status === 200) {
            const responseSize = xhr.responseText.length;
            updateUnifiedProgress(filename, responseSize, responseSize, true);
            
            // Create and inject the resource
            const onLoadSuccess = () => {
                console.log(`Finished: ${filename}`);
                if (resource.type === 'js') {
                    URL.revokeObjectURL(xhr.blobUrl); // Clean up if blob URL was used
                }
                setTimeout(() => loadResourceSequentially(resources, index + 1, 0), CONFIG.RESOURCE_DELAY);
            };
            
            const onLoadError = () => {
                console.error(`Failed to inject ${resource.type}:`, resource.src);
                if (resource.type === 'js') {
                    URL.revokeObjectURL(xhr.blobUrl);
                }
                handleResourceError(resource, filename, retryCount, (newRetryCount, skipToNext) => {
                    if (skipToNext) {
                        loadResourceSequentially(resources, index + 1, 0);
                    } else {
                        loadResourceSequentially(resources, index, newRetryCount);
                    }
                });
            };
            
            const element = createResourceElement(resource, onLoadSuccess, onLoadError);
            
            if (resource.type === 'js') {
                // For JavaScript, create a blob URL from the downloaded content
                const blob = new Blob([xhr.responseText], { type: 'text/javascript' });
                xhr.blobUrl = URL.createObjectURL(blob);
                element.src = xhr.blobUrl;
            }
            
            // Append element to head if it exists (favicon is handled by updating existing element)
            if (element && resource.type !== 'favicon') {
                document.head.appendChild(element);
            }
            
        } else {
            console.error(`Failed to download: ${resource.src} Status: ${xhr.status}`);
            handleResourceError(resource, filename, retryCount, (newRetryCount, skipToNext) => {
                if (skipToNext) {
                    updateUnifiedProgress(filename, 0, fileSizes[filename] || 0, true);
                    loadResourceSequentially(resources, index + 1, 0);
                } else {
                    loadResourceSequentially(resources, index, newRetryCount);
                }
            });
        }
    };
    
    xhr.onerror = function() {
        console.error('Network error loading:', resource.src);
        handleResourceError(resource, filename, retryCount, (newRetryCount, skipToNext) => {
            if (skipToNext) {
                updateUnifiedProgress(filename, 0, fileSizes[filename] || 0, true);
                loadResourceSequentially(resources, index + 1, 0);
            } else {
                loadResourceSequentially(resources, index, newRetryCount);
            }
        });
    };
    
    // Start the download
    xhr.send();
}

// Initialize application after all resources are ready
function initializeAfterResourcesReady() {
    console.log('All resources ready, testing font loading before showing UI...');
    
    // First, test if fonts are actually loaded by creating a test icon
    testFontLoadingBeforeUI();
}

// Test font loading by creating an actual icon element
function testFontLoadingBeforeUI() {
    console.log('Testing font loading with checkmark icon...');
    
    // Update loading status to show font loading
    if (domCache.loadingStatus) {
        domCache.loadingStatus.textContent = 'Loading fonts...';
    }
    if (domCache.currentFile) {
        domCache.currentFile.textContent = 'Verifying icon fonts';
    }
    
    // Find and replace the spinning animation with a green checkmark
    const spinnerElement = document.querySelector('[style*="animation: spin"]');
    if (spinnerElement) {
        // Create a green checkmark icon
        const checkmarkIcon = document.createElement('i');
        checkmarkIcon.className = 'bi bi-check-circle-fill';
        checkmarkIcon.style.fontSize = '32px';
        checkmarkIcon.style.color = '#28a745'; // Bootstrap success green
        checkmarkIcon.style.opacity = '0';
        checkmarkIcon.style.transition = 'opacity 0.5s ease';
        checkmarkIcon.id = 'fontTestIcon';
        
        // Replace the spinner with the checkmark
        spinnerElement.parentNode.replaceChild(checkmarkIcon, spinnerElement);
        
        // Fade in the checkmark
        setTimeout(() => {
            checkmarkIcon.style.opacity = '1';
        }, 100);
        
        // Wait for the element to be processed, then check font loading
        setTimeout(() => {
            waitForFontLoadingCompletion(checkmarkIcon);
        }, 300);
    } else {
        // Fallback: create a hidden test icon if spinner not found
        const testIcon = document.createElement('i');
        testIcon.className = 'bi bi-check-circle-fill';
        testIcon.style.position = 'absolute';
        testIcon.style.left = '-9999px';
        testIcon.style.top = '-9999px';
        testIcon.style.fontSize = '16px';
        testIcon.id = 'fontTestIcon';
        
        document.body.appendChild(testIcon);
        
        setTimeout(() => {
            waitForFontLoadingCompletion(testIcon);
        }, 100);
    }
}

// Wait for font loading to complete using multiple detection methods
function waitForFontLoadingCompletion(testIcon) {
    console.log('Waiting for font loading completion...');
    
    const startTime = Date.now();
    let fontLoadAttempts = 0;
    
    function checkFontLoaded() {
        fontLoadAttempts++;
        const elapsed = Date.now() - startTime;
        
        // Update progress during font loading
        if (domCache.currentFile) {
            domCache.currentFile.textContent = `Font loading... ${Math.round(elapsed/1000)}s`;
        }
        
        // Timeout protection
        if (elapsed > CONFIG.MAX_FONT_WAIT) {
            console.log('Font loading timeout, proceeding anyway...');
            cleanupAndShowUI(testIcon);
            return;
        }
        
        // Method 1: Check document.fonts API
        let documentFontsReady = true;
        if (document.fonts && document.fonts.status) {
            documentFontsReady = document.fonts.status === 'loaded';
            if (!documentFontsReady) {
                console.log(`Document fonts status: ${document.fonts.status} (attempt ${fontLoadAttempts})`);
            }
        }
        
        // Method 2: Check computed style of test icon
        let iconStyleReady = false;
        try {
            const computedStyle = window.getComputedStyle(testIcon, ':before');
            const fontFamily = computedStyle.getPropertyValue('font-family');
            iconStyleReady = fontFamily && fontFamily.includes('bootstrap-icons');
            
            if (!iconStyleReady) {
                console.log(`Icon font-family: ${fontFamily} (attempt ${fontLoadAttempts})`);
            }
        } catch (e) {
            console.log('Could not check icon computed style:', e.message);
        }
        
        // Method 3: Check for recent font network activity
        let noRecentFontActivity = true;
        try {
            const resourceEntries = performance.getEntriesByType('resource');
            const recentFonts = resourceEntries.filter(entry => {
                const isFont = entry.name.includes('woff') || entry.name.includes('ttf') || 
                              entry.name.includes('bootstrap-icons');
                const isVeryRecent = (performance.now() - entry.startTime) < 1000; // Last 1 second
                const isIncomplete = entry.responseEnd === 0;
                
                return isFont && isVeryRecent && isIncomplete;
            });
            
            noRecentFontActivity = recentFonts.length === 0;
            if (!noRecentFontActivity) {
                console.log(`Recent incomplete font activity: ${recentFonts.length} resources (attempt ${fontLoadAttempts})`);
            }
        } catch (e) {
            console.log('Could not check font network activity:', e.message);
        }
        
        // All methods must indicate fonts are ready
        const allFontsReady = documentFontsReady && iconStyleReady && noRecentFontActivity;
        
        if (allFontsReady) {
            console.log(`All font loading checks passed after ${elapsed}ms (${fontLoadAttempts} attempts)`);
            cleanupAndShowUI(testIcon);
        } else {
            console.log(`Font loading not complete, waiting... (${elapsed}ms, attempt ${fontLoadAttempts})`);
            setTimeout(checkFontLoaded, CONFIG.FONT_CHECK_INTERVAL);
        }
    }
    
    // Start checking after a brief delay
    setTimeout(checkFontLoaded, 100);
}

// Show the UI (checkmark stays as part of the loading interface)
function cleanupAndShowUI(testIcon) {
    console.log('Font verification complete, showing UI...');
    
    // No need to remove test icon since it's now part of the UI as the checkmark
    // The loading overlay will be hidden, taking the checkmark with it
    
    // Now proceed with showing the UI
    setTimeout(function() {
        // Trigger main.js initialization
        if (window.initializeApplication && typeof window.initializeApplication === 'function') {
            console.log('Calling initializeApplication...');
            window.initializeApplication();
        } else {
            console.log('initializeApplication not found, waiting...');
            setTimeout(function() {
                if (window.initializeApplication && typeof window.initializeApplication === 'function') {
                    console.log('Calling initializeApplication (delayed)...');
                    window.initializeApplication();
                }
            }, 200);
        }
        
        // Hide loading overlay and show interface
        if (domCache.loadingOverlay) {
            domCache.loadingOverlay.style.display = 'none';
        }
        if (domCache.mainContainer) {
            domCache.mainContainer.style.display = 'block';
        }
        
        // Since we've already waited for fonts, proceed directly to status updates
        console.log('UI shown with fonts ready, starting status updates...');
        setTimeout(() => {
            startStatusUpdatesAfterFontStability();
        }, 500); // Brief delay to let UI settle
        
    }, CONFIG.UI_SHOW_DELAY);
}

// Wait for any pending requests triggered when UI becomes visible
function waitForActualFontLoadingAfterUIShow() {
    console.log('Starting ESP32-optimized network activity detection...');
    
    let startTime = Date.now();
    let lastNetworkActivityTime = startTime;
    let fontLoadingDetected = false;
    
    // Track active XMLHttpRequests
    const originalXHROpen = XMLHttpRequest.prototype.open;
    const originalXHRSend = XMLHttpRequest.prototype.send;
    let activeXHRCount = 0;
    
    // Override XHR to track active requests
    XMLHttpRequest.prototype.open = function(...args) {
        activeXHRCount++;
        console.log(`XHR opened, active count: ${activeXHRCount}`);
        return originalXHROpen.apply(this, args);
    };
    
    XMLHttpRequest.prototype.send = function(...args) {
        this.addEventListener('loadend', () => {
            activeXHRCount = Math.max(0, activeXHRCount - 1);
            console.log(`XHR completed, active count: ${activeXHRCount}`);
        });
        return originalXHRSend.apply(this, args);
    };
    
    // Network activity detection functions
    const networkDetection = {
        isFontStillLoading() {
            try {
                if (document.fonts && document.fonts.status) {
                    const fontLoading = document.fonts.status === 'loading';
                    if (fontLoading) {
                        console.log('Document fonts still loading');
                        fontLoadingDetected = true;
                        return true;
                    }
                }
                
                const resourceEntries = performance.getEntriesByType('resource');
                const recentFonts = resourceEntries.filter(entry => {
                    const isFont = entry.name.includes('woff') || entry.name.includes('ttf') || 
                                  entry.name.includes('bootstrap-icons');
                    const isRecent = (performance.now() - entry.startTime) < CONFIG.FONT_TIMEOUT;
                    const isIncomplete = entry.responseEnd === 0 || 
                                       (entry.transferSize === 0 && entry.encodedBodySize === 0);
                    
                    if (isFont && isRecent && isIncomplete) {
                        console.log(`Incomplete font detected: ${entry.name}`);
                        fontLoadingDetected = true;
                        return true;
                    }
                    return false;
                });
                
                return recentFonts.length > 0;
            } catch (e) {
                console.log('Font loading check failed:', e.message);
                return false;
            }
        },
        
        hasActiveConnections() {
            try {
                if (activeXHRCount > 0) {
                    console.log(`Active XHR connections: ${activeXHRCount}`);
                    return true;
                }
                
                if (window.activeFetchCount && window.activeFetchCount > 0) {
                    console.log(`Active fetch requests: ${window.activeFetchCount}`);
                    return true;
                }
                
                return false;
            } catch (e) {
                return false;
            }
        },
        
        isConnectionLikelySlow() {
            try {
                if (navigator.connection) {
                    const conn = navigator.connection;
                    const isSlow = conn.effectiveType === 'slow-2g' || 
                                  conn.effectiveType === '2g' || 
                                  conn.effectiveType === '3g' ||
                                  conn.downlink < 2;
                    if (isSlow) {
                        console.log(`🐌 Slow ESP32 connection: ${conn.effectiveType}, downlink: ${conn.downlink}Mbps`);
                        return true;
                    }
                }
                return false;
            } catch (e) {
                return false;
            }
        },
        
        isDocumentStillLoading() {
            const isLoading = document.readyState === 'loading';
            if (isLoading) {
                console.log('Document still in loading state');
            }
            return isLoading;
        }
    };
    
    function checkNetwork() {
        const currentTime = Date.now();
        const elapsed = currentTime - startTime;
        
        // Timeout protection
        if (elapsed > CONFIG.DETECTION_TIMEOUT) {
            console.log('ESP32 network detection timeout reached, proceeding...');
            restoreXHRAndProceed();
            return;
        }
        
        // Run all detection methods
        const anyActivity = networkDetection.isFontStillLoading() || 
                           networkDetection.hasActiveConnections() || 
                           networkDetection.isConnectionLikelySlow() || 
                           networkDetection.isDocumentStillLoading();
        
        if (anyActivity) {
            lastNetworkActivityTime = currentTime;
            console.log(`ESP32 network activity detected at ${elapsed}ms`);
        }
        
        // Special handling if we detected font loading
        if (fontLoadingDetected && elapsed < CONFIG.FONT_TIMEOUT) {
            console.log(`Font loading detected, extending wait period (${elapsed}/${CONFIG.FONT_TIMEOUT}ms)`);
            setTimeout(checkNetwork, CONFIG.CHECK_INTERVAL);
            return;
        }
        
        // Check if we've had sufficient stability
        const timeSinceLastActivity = currentTime - lastNetworkActivityTime;
        
        if (timeSinceLastActivity >= CONFIG.STABILITY_PERIOD) {
            console.log(`ESP32 network stable for ${timeSinceLastActivity}ms, proceeding with status updates`);
            restoreXHRAndProceed();
        } else {
            setTimeout(checkNetwork, CONFIG.CHECK_INTERVAL);
        }
    }
    
    function restoreXHRAndProceed() {
        // Restore XMLHttpRequest
        XMLHttpRequest.prototype.open = originalXHROpen;
        XMLHttpRequest.prototype.send = originalXHRSend;
        startStatusUpdatesAfterFontStability();
    }
    
    // Start monitoring after small delay
    setTimeout(checkNetwork, CONFIG.NETWORK_CHECK_DELAY);
}

// Start status updates after confirming font loading is complete
function startStatusUpdatesAfterFontStability() {
    console.log('Starting status updates after font loading stability confirmed...');
    updateStatus();
    startRegularUpdates();
    updateWiFiConfigUI();
}

// Start sequential loading when page is fully loaded
function startSequentialLoading() {
    console.log('Page fully loaded, starting sequential resource loading...');
    
    // Initialize DOM cache early
    initDOMCache();
    
    // Use the globally defined resources configuration
    const resourcesToLoad = RESOURCES_TO_LOAD;
    
    // Fetch file sizes from server
    fetch('/getJsonFiles')
        .then(response => response.json())
        .then(data => {
            console.log('File list loaded from server');
            initializeProgressTracking(data, resourcesToLoad);
            loadResourceSequentially(resourcesToLoad);
        })
        .catch(error => {
            console.error('Failed to fetch file sizes:', error);
            // Fallback: start loading without file sizes
            initializeProgressTracking(null, resourcesToLoad);
            loadResourceSequentially(resourcesToLoad);
        });
}

// Initialize progress tracking variables
function initializeProgressTracking(serverData, resourcesToLoad) {
    // Create a map of filenames to sizes from the server response
    fileSizes = {};
    if (serverData && serverData.files) {
        serverData.files.forEach(file => {
            // Extract filename from path, e.g., "/css/bootstrap.min.css" -> "bootstrap.min.css"
            const filename = file.path.substring(file.path.lastIndexOf('/') + 1);
            fileSizes[filename] = file.size;
        });
    }
    
    // Initialize progress tracking
    totalBytes = 0;
    completedBytes = 0;
    currentFileProgress = {};
    
    // Calculate total bytes for resources to be loaded
    resourcesToLoad.forEach(resource => {
        const filename = resource.src.split('/').pop();
        const fileSize = fileSizes[filename] || 0;
        totalBytes += fileSize;
    });
    
    // Initialize progress display
    updateUnifiedProgress('Starting...', 0, 0, false);
}

// Start sequential loading when page is fully loaded
window.addEventListener('load', () => {
    startSequentialLoading();
});
