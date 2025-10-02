#include <WiFi.h>
#include <WebServer.h>
#include <Stepper.h>
#include <SPIFFS.h>

// ============================================
// CONFIGURATION - CHANGE THESE VALUES
// ============================================
const char* ssid = "Broomys iPhone";
const char* password = "kaibroomfieldwifi";

// Pin definitions for 28BYJ-48 stepper motor
#define STEPPER_PIN1 19
#define STEPPER_PIN2 18
#define STEPPER_PIN3 5
#define STEPPER_PIN4 17

// Sensor and control pins
#define IR_SENSOR_PIN 16
#define BUTTON_PIN 4
#define BUZZER_PIN 15
#define STATUS_LED 2
#define DISPENSE_LED 13

// Motor configuration
#define STEPS_PER_REV 2048  // 28BYJ-48 full revolution
#define POCKETS 4           // Number of pockets in drum
#define STEPS_PER_POCKET (STEPS_PER_REV / POCKETS)
#define MOTOR_SPEED 10      // RPM

// ============================================
// GLOBAL OBJECTS AND VARIABLES
// ============================================
Stepper motor(STEPS_PER_REV, STEPPER_PIN1, STEPPER_PIN3, STEPPER_PIN2, STEPPER_PIN4);
WebServer server(80);

// Statistics
int totalPillsDispensed = 0;
int successfulDispenses = 0;
int failedDispenses = 0;
unsigned long lastDispenseTime = 0;
bool lastSensorState = HIGH;
int sessionPillsDetected = 0;

// System status
bool systemEnabled = true;
unsigned long systemUptime = 0;

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=================================");
  Serial.println("ESP32 Pill Dispenser - Phase 2");
  Serial.println("=================================\n");
  
  // Initialize SPIFFS for storing statistics
  if (!SPIFFS.begin(true)) {
    Serial.println("⚠️  SPIFFS Mount Failed");
  } else {
    Serial.println("✓ SPIFFS Mounted");
    loadStatistics();
  }
  
  // Initialize pins
  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(DISPENSE_LED, OUTPUT);
  
  // Initialize motor
  motor.setSpeed(MOTOR_SPEED);
  
  // Connect to WiFi
  connectToWiFi();
  
  // Setup web server routes
  setupWebServer();
  
  // Start server
  server.begin();
  
  // Success indication
  digitalWrite(STATUS_LED, HIGH);
  tone(BUZZER_PIN, 1000, 200);
  delay(300);
  tone(BUZZER_PIN, 1500, 200);
  
  Serial.println("\n✓ System Ready!");
  Serial.println("=================================\n");
  
  systemUptime = millis();
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
  server.handleClient();
  checkManualButton();
  blinkStatusLED();
  delay(50);
}

// ============================================
// WIFI CONNECTION
// ============================================
void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n✗ WiFi Connection Failed");
    Serial.println("Device will work with manual button only");
  }
}

// ============================================
// WEB SERVER SETUP
// ============================================
void setupWebServer() {
  // Main page
  server.on("/", HTTP_GET, handleRoot);
  
  // Serve CSS
  server.on("/styles/style.css", HTTP_GET, handleCSS);
  
  // Serve JavaScript
  server.on("/script.js", HTTP_GET, handleJS);
  
  // API endpoints
  server.on("/api/dispense", HTTP_POST, handleDispense);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.on("/api/system", HTTP_POST, handleSystemControl);
  
  // 404 handler
  server.onNotFound([]() {
    server.send(404, "text/plain", "404: Not Found");
  });
}

// ============================================
// WEB PAGE
// ============================================
void handleRoot() {
  String html = R"rawliteral(<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Pill Dispenser Control</title>
    <link rel="stylesheet" href="/style.css" />
  </head>
  <body>
    <div class="container">
      <div class="card">
        <h1>Pill Dispenser</h1>
        <div class="subtitle">Web Control</div>

        <div class="system-status">
          <span class="status-indicator online" id="statusIndicator"></span>
          <span id="statusText">System Online</span>
        </div>

        <div id="alertBox" class="alert"></div>

        <div class="status-grid">
          <div class="status-item">
            <div class="status-label">Total Dispensed</div>
            <div class="status-value" id="totalDispensed">0</div>
          </div>
          <div class="status-item">
            <div class="status-label">Success Rate</div>
            <div class="status-value" id="successRate">0%</div>
          </div>
          <div class="status-item">
            <div class="status-label">Successful</div>
            <div class="status-value" style="color: #4caf50" id="successful">0</div>
          </div>
          <div class="status-item">
            <div class="status-label">Failed</div>
            <div class="status-value" style="color: #f44336" id="failed">0</div>
          </div>
        </div>

        <button class="dispense-btn" onclick="dispense()" id="dispenseBtn">
          DISPENSE PILL
        </button>

        <button class="btn-secondary" onclick="toggleSystem()">
          <span id="systemToggleText">Disable System</span>
        </button>

        <button class="btn-secondary" onclick="resetStats()">
          Reset Statistics
        </button>

        <div class="accuracy" id="accuracyInfo"></div>
      </div>

      <div class="card">
        <h2 style="margin-bottom: 15px">📊 System Info</h2>
        <div style="font-size: 14px; line-height: 1.8; color: #666">
          <p>
            <strong>Last Dispense:</strong> <span id="lastDispense">Never</span>
          </p>
          <p>
            <strong>WiFi Signal:</strong> <span id="wifiSignal">-72 dBm</span>
          </p>
          <p><strong>Uptime:</strong> <span id="uptime">0h 0m</span></p>
          <p>
            <strong>Free Memory:</strong> <span id="freeMemory">45 KB</span>
          </p>
        </div>
      </div>
    </div>

    <script src="/script.js"></script>
  </body>
</html>)rawliteral";
  
  server.send(200, "text/html", html);
}

// ============================================
// CSS HANDLER
// ============================================
void handleCSS() {
  String css = R"rawliteral(* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
  }
  
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", "Times New Roman",
      Times, serif, sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    min-height: 100vh;
    padding: 20px;
  }
  
  .container {
    max-width: 600px;
    margin: 0 auto;
  }
  
  .card {
    background: white;
    border-radius: 20px;
    padding: 30px;
    margin: 20px 0;
    box-shadow: 0 10px 40px rgba(0, 0, 0, 0.2);
  }
  
  h1 {
    color: #333;
    font-size: 32px;
    margin-bottom: 10px;
    text-align: center;
  }
  
  .subtitle {
    text-align: center;
    color: #666;
    margin-bottom: 30px;
    font-size: 14px;
  }
  
  .status-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 15px;
    margin-bottom: 30px;
  }
  
  .status-item {
    text-align: center;
    padding: 15px;
    background: #f8f9fa;
    border-radius: 10px;
  }
  
  .status-label {
    font-size: 12px;
    color: #666;
    text-transform: uppercase;
    letter-spacing: 1px;
  }
  
  .status-value {
    font-size: 28px;
    font-weight: bold;
    color: #333;
    margin-top: 5px;
  }
  
  .dispense-btn {
    width: 100%;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    border: none;
    padding: 20px;
    font-size: 24px;
    font-weight: bold;
    border-radius: 15px;
    cursor: pointer;
    transition: transform 0.2s, box-shadow 0.2s;
    margin-bottom: 15px;
  }
  
  .dispense-btn:hover {
    transform: translateY(-2px);
    box-shadow: 0 5px 20px rgba(102, 126, 234, 0.4);
  }
  
  .dispense-btn:active {
    transform: translateY(0);
  }
  
  .dispense-btn:disabled {
    background: #ccc;
    cursor: not-allowed;
    transform: none;
  }
  
  .btn-secondary {
    width: 100%;
    background: white;
    color: #667eea;
    border: 2px solid #667eea;
    padding: 12px;
    font-size: 16px;
    font-weight: bold;
    border-radius: 10px;
    cursor: pointer;
    margin: 5px 0;
    transition: all 0.2s;
  }
  
  .btn-secondary:hover {
    background: #667eea;
    color: white;
  }
  
  .system-status {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 10px;
    padding: 15px;
    background: #f8f9fa;
    border-radius: 10px;
    margin-bottom: 20px;
  }
  
  .status-indicator {
    width: 12px;
    height: 12px;
    border-radius: 50%;
    animation: pulse 2s infinite;
  }
  
  .online {
    background: #4caf50;
  }
  
  .offline {
    background: #f44336;
    animation: none;
  }
  
  @keyframes pulse {
    0%,
    100% {
      opacity: 1;
    }
    50% {
      opacity: 0.5;
    }
  }
  
  .alert {
    padding: 15px;
    border-radius: 10px;
    margin: 15px 0;
    display: none;
  }
  
  .alert.success {
    background: #d4edda;
    color: #155724;
    border: 1px solid #c3e6cb;
  }
  
  .alert.error {
    background: #f8d7da;
    color: #721c24;
    border: 1px solid #f5c6cb;
  }
  
  .demo-badge {
    background: #ff9800;
    color: white;
    padding: 5px 15px;
    border-radius: 20px;
    font-size: 12px;
    display: inline-block;
    margin-bottom: 15px;
  }
  
  .accuracy {
    text-align: center;
    font-size: 14px;
    color: #666;
    margin-top: 10px;
  }
  
  .accuracy-good {
    color: #4caf50;
  }
  
  .accuracy-warning {
    color: #ff9800;
  }
  
  .accuracy-bad {
    color: #f44336;
  })rawliteral";
  
  server.send(200, "text/css", css);
}

// ============================================
// JAVASCRIPT HANDLER
// ============================================
void handleJS() {
  String js = R"rawliteral(let isDispensing = false;
let systemEnabled = true;

function updateUI(data) {
  document.getElementById('totalDispensed').textContent = data.totalDispensed;
  document.getElementById('successful').textContent = data.successful;
  document.getElementById('failed').textContent = data.failed;

  let successRate = data.totalDispensed > 0
    ? Math.round((data.successful / data.totalDispensed) * 100)
    : 0;
  document.getElementById('successRate').textContent = successRate + '%';

  let accuracyEl = document.getElementById('accuracyInfo');
  if (data.totalDispensed > 0) {
    if (successRate >= 95) {
      accuracyEl.className = 'accuracy accuracy-good';
      accuracyEl.textContent = '✓ Excellent accuracy';
    } else if (successRate >= 85) {
      accuracyEl.className = 'accuracy accuracy-warning';
      accuracyEl.textContent = '⚠ Accuracy needs improvement';
    } else {
      accuracyEl.className = 'accuracy accuracy-bad';
      accuracyEl.textContent = '✗ Poor accuracy - check mechanism';
    }
  }

  document.getElementById('lastDispense').textContent = data.lastDispense || 'Never';
  document.getElementById('wifiSignal').textContent = data.wifiSignal + ' dBm';
  document.getElementById('uptime').textContent = data.uptime;
  document.getElementById('freeMemory').textContent = data.freeMemory + ' KB';

  systemEnabled = data.systemEnabled;
  document.getElementById('systemToggleText').textContent =
    systemEnabled ? 'Disable System' : 'Enable System';

  if (!systemEnabled) {
    document.getElementById('statusText').textContent = 'System Disabled';
    document.getElementById('statusIndicator').className = 'status-indicator offline';
  } else {
    document.getElementById('statusText').textContent = 'System Online';
    document.getElementById('statusIndicator').className = 'status-indicator online';
  }
}

function showAlert(message, type) {
  let alertBox = document.getElementById('alertBox');
  alertBox.textContent = message;
  alertBox.className = 'alert ' + type;
  alertBox.style.display = 'block';
  setTimeout(() => {
    alertBox.style.display = 'none';
  }, 5000);
}

function dispense() {
  if (isDispensing) return;
  if (!systemEnabled) {
    showAlert('System is disabled. Enable it first.', 'error');
    return;
  }

  isDispensing = true;
  let btn = document.getElementById('dispenseBtn');
  btn.disabled = true;
  btn.textContent = 'DISPENSING...';

  fetch('/api/dispense', { method: 'POST' })
    .then(response => response.json())
    .then(data => {
      if (data.success) {
        showAlert('✓ Pill dispensed successfully!', 'success');
      } else {
        showAlert('✗ Dispense failed: ' + data.message, 'error');
      }
      refreshStatus();
    })
    .catch(error => {
      showAlert('✗ Connection error', 'error');
      console.error('Error:', error);
    })
    .finally(() => {
      isDispensing = false;
      btn.disabled = false;
      btn.textContent = 'DISPENSE PILL';
    });
}

function toggleSystem() {
  let newState = !systemEnabled;

  fetch('/api/system', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ enabled: newState })
  })
  .then(response => response.json())
  .then(data => {
    if (data.success) {
      showAlert(
        newState ? '✓ System enabled' : '⚠ System disabled',
        newState ? 'success' : 'error'
      );
      refreshStatus();
    }
  })
  .catch(error => {
    showAlert('✗ Connection error', 'error');
  });
}

function resetStats() {
  if (!confirm('Reset all statistics? This cannot be undone.')) return;

  fetch('/api/reset', { method: 'POST' })
    .then(response => response.json())
    .then(data => {
      if (data.success) {
        showAlert('✓ Statistics reset', 'success');
        refreshStatus();
      }
    })
    .catch(error => {
      showAlert('✗ Connection error', 'error');
    });
}

function refreshStatus() {
  fetch('/api/status')
    .then(response => response.json())
    .then(data => updateUI(data))
    .catch(error => {
      console.error('Error:', error);
      document.getElementById('statusText').textContent = 'Connection Error';
      document.getElementById('statusIndicator').className = 'status-indicator offline';
    });
}

refreshStatus();
setInterval(refreshStatus, 10000);)rawliteral";
  
  server.send(200, "application/javascript", js);
}

// ============================================
// API HANDLERS
// ============================================
void handleStatus() {
  String json = "{";
  json += "\"totalDispensed\":" + String(totalPillsDispensed) + ",";
  json += "\"successful\":" + String(successfulDispenses) + ",";
  json += "\"failed\":" + String(failedDispenses) + ",";
  
  if (lastDispenseTime > 0) {
    unsigned long elapsed = (millis() - lastDispenseTime) / 1000;
    json += "\"lastDispense\":\"";
    if (elapsed < 60) {
      json += String(elapsed) + " seconds ago";
    } else if (elapsed < 3600) {
      json += String(elapsed / 60) + " minutes ago";
    } else {
      json += String(elapsed / 3600) + " hours ago";
    }
    json += "\",";
  } else {
    json += "\"lastDispense\":\"Never\",";
  }
  
  json += "\"wifiSignal\":" + String(WiFi.RSSI()) + ",";
  
  unsigned long uptime = (millis() - systemUptime) / 1000;
  json += "\"uptime\":\"";
  json += String(uptime / 3600) + "h " + String((uptime % 3600) / 60) + "m\",";
  
  json += "\"freeMemory\":" + String(ESP.getFreeHeap() / 1024) + ",";
  json += "\"systemEnabled\":" + String(systemEnabled ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleDispense() {
  if (!systemEnabled) {
    server.send(200, "application/json", 
      "{\"success\":false,\"message\":\"System is disabled\"}");
    return;
  }
  
  Serial.println("\n--- WEB DISPENSE REQUEST ---");
  bool success = dispensePill();
  
  if (success) {
    server.send(200, "application/json", 
      "{\"success\":true,\"message\":\"Pill dispensed\"}");
  } else {
    server.send(200, "application/json", 
      "{\"success\":false,\"message\":\"No pill detected\"}");
  }
}

void handleReset() {
  totalPillsDispensed = 0;
  successfulDispenses = 0;
  failedDispenses = 0;
  lastDispenseTime = 0;
  
  saveStatistics();
  
  Serial.println("✓ Statistics reset");
  server.send(200, "application/json", "{\"success\":true}");
}

void handleSystemControl() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    if (body.indexOf("\"enabled\":true") > 0) {
      systemEnabled = true;
      Serial.println("✓ System ENABLED");
    } else if (body.indexOf("\"enabled\":false") > 0) {
      systemEnabled = false;
      Serial.println("⚠ System DISABLED");
    }
    
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"success\":false}");
  }
}

// ============================================
// CORE DISPENSING LOGIC
// ============================================
bool dispensePill() {
  Serial.println("💊 Dispensing pill...");
  digitalWrite(DISPENSE_LED, HIGH);
  
  sessionPillsDetected = 0;
  
  Serial.println("   Rotating drum...");
  motor.step(STEPS_PER_POCKET);
  
  unsigned long startTime = millis();
  while (millis() - startTime < 3000) {
    checkSensor();
    if (sessionPillsDetected > 0) break;
    delay(10);
  }
  
  digitalWrite(DISPENSE_LED, LOW);
  
  bool success = (sessionPillsDetected > 0);
  
  if (success) {
    Serial.println("✓ SUCCESS: Pill detected");
    successfulDispenses++;
    tone(BUZZER_PIN, 1000, 200);
  } else {
    Serial.println("✗ FAILURE: No pill detected");
    failedDispenses++;
    for (int i = 0; i < 3; i++) {
      tone(BUZZER_PIN, 2000, 100);
      delay(150);
    }
  }
  
  totalPillsDispensed++;
  lastDispenseTime = millis();
  
  saveStatistics();
  
  float accuracy = (float)successfulDispenses / totalPillsDispensed * 100;
  Serial.print("   Accuracy: ");
  Serial.print(accuracy, 1);
  Serial.println("%");
  Serial.println("----------------------------\n");
  
  return success;
}

void checkSensor() {
  bool currentState = digitalRead(IR_SENSOR_PIN);
  
  if (lastSensorState == HIGH && currentState == LOW) {
    sessionPillsDetected++;
    Serial.print("   🔔 Pill detected! Count: ");
    Serial.println(sessionPillsDetected);
  }
  
  lastSensorState = currentState;
}

// ============================================
// MANUAL BUTTON
// ============================================
void checkManualButton() {
  static unsigned long lastButtonPress = 0;
  
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (millis() - lastButtonPress > 500) {
      if (systemEnabled) {
        Serial.println("\n🖱️  MANUAL BUTTON PRESSED");
        dispensePill();
      } else {
        Serial.println("⚠️  System disabled - button ignored");
        tone(BUZZER_PIN, 500, 200);
      }
      lastButtonPress = millis();
    }
    while(digitalRead(BUTTON_PIN) == LOW) delay(10);
  }
}

// ============================================
// STATUS LED BLINK
// ============================================
void blinkStatusLED() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  if (millis() - lastBlink > 2000) {
    ledState = !ledState;
    digitalWrite(STATUS_LED, ledState ? HIGH : LOW);
    lastBlink = millis();
  }
}

// ============================================
// STATISTICS PERSISTENCE
// ============================================
void saveStatistics() {
  if (!SPIFFS.begin()) return;
  
  File file = SPIFFS.open("/stats.txt", "w");
  if (file) {
    file.println(totalPillsDispensed);
    file.println(successfulDispenses);
    file.println(failedDispenses);
    file.close();
  }
}

void loadStatistics() {
  if (!SPIFFS.begin()) return;
  
  if (SPIFFS.exists("/stats.txt")) {
    File file = SPIFFS.open("/stats.txt", "r");
    if (file) {
      totalPillsDispensed = file.readStringUntil('\n').toInt();
      successfulDispenses = file.readStringUntil('\n').toInt();
      failedDispenses = file.readStringUntil('\n').toInt();
      file.close();
      
      Serial.println("✓ Statistics loaded from memory");
      Serial.printf("   Total: %d, Success: %d, Failed: %d\n", 
        totalPillsDispensed, successfulDispenses, failedDispenses);
    }
  }
}