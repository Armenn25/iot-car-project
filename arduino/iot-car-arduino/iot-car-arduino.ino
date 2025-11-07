#include <WiFi.h>
#include <WiFiMulti.h>  // <-- Za multiple WiFi mreže
#include <WebServer.h>
#include <Preferences.h>  // <-- Za trajno čuvanje mreža
#include <WebSocketsClient.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>

#define STATUS_LED 2

// --- Server ---
const char* websocket_server_host = "iotcarbe.xyz";
const uint16_t websocket_server_port = 443;
const char* websocket_server_path = "/carhub";
const char* deviceId = "ESP32-Auto-001";

// --- Pinout ---
const int motorEnablePin = 27, motorIn1Pin = 25, motorIn2Pin = 26;
const int servoPin = 13;
const int frontLedPin = 16, reverseLedPin = 17;
const int batteryPin = 35, acsPin = 34, hallSensorPin = 33;

// --- PWM settings ---
const int motorPwmFreq = 5000;
const int motorPwmResolution = 8;

// --- RPM / Speed ---
const float wheelDiameterCm = 4.0;
const float pulsesPerRevolution = 1.0;
const float gearRatio = 48.0;
const float wheelCircumference = (wheelDiameterCm * 3.14159) / 100.0;

// Hall senzor varijable
const int hallThreshold = 2200;
bool magnetDetected = false;
volatile unsigned long lastPulseTime = 0;
volatile unsigned long pulseInterval = 0;
float currentRpm = 0, currentSpeed = 0, motorRpm = 0;

unsigned long lastTelemetrySend = 0;
const long telemetryInterval = 2000;

// --- Battery / Consumption ---
const float R1_BAT = 20000.0, R2_BAT = 10000.0;
const float R1_ACS = 10000.0, R2_ACS = 2200.0;
const float ADC_REFERENCE = 3.3;
const int adcResolution = 4095;

// ACS712-05A
const float ACS_SENSITIVITY = 185.0;
float acsOffset_mV = 2500.0;

// Postavke baterije
const float BATTERY_CAPACITY_mAh = 6000.0; 
const float maxBatteryVoltage = 9.1;
const float minBatteryVoltage = 7.0;

float batteryVoltage = 0;
float batteryPercentage = 0;
float current = 0;
float battery_mAhRemaining = BATTERY_CAPACITY_mAh;

// --- Control Variables ---
int currentPwmValue = 0;
bool isReversing = false;
bool lightsOn = false;

// --- Objects ---
WebSocketsClient webSocket;
Servo servo;
bool isWebSocketConnected = false;
WebServer server(80);
WiFiMulti wifiMulti;  // <-- Za multiple WiFi mreže
Preferences preferences;  // <-- Za čuvanje u flash memoriju

// --- Multi-WiFi Management ---
#define MAX_WIFI_NETWORKS 10
bool isAPMode = false;
bool isConnectedToWiFi = false;
unsigned long lastWiFiCheck = 0;
const long wifiCheckInterval = 30000; // Provjera svakih 30s

// --- Function Prototypes ---
void calibrateAcsOffset();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void sendTelemetry();
void calculateRpmAndSpeed();
float readBatteryVoltage();
float readCurrent();
void goForward(int speed);
void goBackward(int speed);
void stopMotors();
void turnLeft();
void turnRight();
void setLights(bool on);

// Multi-WiFi funkcije
void loadSavedNetworks();
void saveNetwork(String ssid, String password);
void removeNetwork(String ssid);
bool connectToKnownNetworks();
void startAPMode();
void setupWebServer();
void handleRoot();
void handleScan();
void handleConnect();
void handleSavedNetworks();
void handleRemoveNetwork();
void handleStatus();
void updateLEDStatus();

void setup() {
    Serial.begin(115200);
    pinMode(STATUS_LED, OUTPUT);
    pinMode(frontLedPin, OUTPUT);
    pinMode(reverseLedPin, OUTPUT);
    pinMode(hallSensorPin, INPUT);
    pinMode(motorIn1Pin, OUTPUT);
    pinMode(motorIn2Pin, OUTPUT);

    analogReadResolution(12);
    ledcAttach(motorEnablePin, motorPwmFreq, motorPwmResolution);

    ESP32PWM::allocateTimer(1);
    servo.setPeriodHertz(50);
    servo.attach(servoPin, 500, 2400);
    servo.write(90);

    stopMotors();
    calibrateAcsOffset();

    // ═══════════════════════════════════════════════════════════
    // MULTI-WIFI SISTEM
    // ═══════════════════════════════════════════════════════════
    
    preferences.begin("wifi-config", false);
    
    Serial.println("\n═══════════════════════════════════════");
    Serial.println("   ESP32 Auto - Multi-WiFi System");
    Serial.println("═══════════════════════════════════════");
    
    // Učitaj sačuvane mreže
    loadSavedNetworks();
    
    // Pokušaj povezivanje na poznate mreže
    Serial.println("\n🔍 Skeniram poznate WiFi mreže...");
    if(connectToKnownNetworks()) {
        Serial.println("✓ Povezan na poznatu mrežu!");
        Serial.print("  SSID: ");
        Serial.println(WiFi.SSID());
        Serial.print("  IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("  Signal: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        
        isConnectedToWiFi = true;
        isAPMode = false;
        
        // Brzo blinkanje - uspješna konekcija
        for(int i = 0; i < 6; i++) {
            digitalWrite(STATUS_LED, HIGH);
            delay(100);
            digitalWrite(STATUS_LED, LOW);
            delay(100);
        }
    } else {
        Serial.println("⚠️  Nema poznatih mreža u blizini!");
        Serial.println("📡 Pokrećem AP mod za konfiguraciju...");
        startAPMode();
    }
    
    // Pokreni web server
    setupWebServer();
    server.begin();
    Serial.println("✓ Web server pokrenut!");
    Serial.println("═══════════════════════════════════════\n");

    // WebSocket konekcija (samo ako smo povezani)
    if(isConnectedToWiFi) {
        webSocket.beginSSL(websocket_server_host, websocket_server_port, websocket_server_path);
        webSocket.onEvent(webSocketEvent);
        webSocket.setReconnectInterval(5000);
    }
}

void loop() {
    server.handleClient();
    calculateRpmAndSpeed();
    updateLEDStatus();
    
    // Ako smo povezani na WiFi, rukovodimo WebSocket-om
    if(isConnectedToWiFi) {
        webSocket.loop();
        
        if (isWebSocketConnected && (millis() - lastTelemetrySend > telemetryInterval)) {
            lastTelemetrySend = millis();
            sendTelemetry();
        }
    }
    
    // Povremeno provjeri da li postoji bolja mreža
    if(!isAPMode && millis() - lastWiFiCheck > wifiCheckInterval) {
        lastWiFiCheck = millis();
        
        if(WiFi.status() != WL_CONNECTED) {
            Serial.println("⚠️  Veza izgubljena! Pokušavam reconnect...");
            if(connectToKnownNetworks()) {
                Serial.println("✓ Ponovo povezan!");
                isConnectedToWiFi = true;
                
                // Reconnect WebSocket
                webSocket.beginSSL(websocket_server_host, websocket_server_port, websocket_server_path);
            } else {
                Serial.println("✗ Nema dostupnih mreža. Prelazim u AP mod.");
                isConnectedToWiFi = false;
                startAPMode();
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════
// MULTI-WIFI MANAGEMENT FUNCTIONS
// ═══════════════════════════════════════════════════════════

void loadSavedNetworks() {
    int networkCount = preferences.getInt("count", 0);
    Serial.printf("📂 Učitavam %d sačuvanih mreža...\n", networkCount);
    
    for(int i = 0; i < networkCount && i < MAX_WIFI_NETWORKS; i++) {
        String ssidKey = "ssid" + String(i);
        String passKey = "pass" + String(i);
        
        String ssid = preferences.getString(ssidKey.c_str(), "");
        String password = preferences.getString(passKey.c_str(), "");
        
        if(ssid.length() > 0) {
            wifiMulti.addAP(ssid.c_str(), password.c_str());
            Serial.printf("  %d. %s\n", i+1, ssid.c_str());
        }
    }
}

void saveNetwork(String ssid, String password) {
    int networkCount = preferences.getInt("count", 0);
    
    // Provjeri da li mreža već postoji
    for(int i = 0; i < networkCount; i++) {
        String ssidKey = "ssid" + String(i);
        String existingSsid = preferences.getString(ssidKey.c_str(), "");
        
        if(existingSsid == ssid) {
            // Ažuriraj lozinku ako se promijenila
            String passKey = "pass" + String(i);
            preferences.putString(passKey.c_str(), password);
            Serial.printf("✓ Ažurirana lozinka za: %s\n", ssid.c_str());
            return;
        }
    }
    
    // Dodaj novu mrežu
    if(networkCount < MAX_WIFI_NETWORKS) {
        String ssidKey = "ssid" + String(networkCount);
        String passKey = "pass" + String(networkCount);
        
        preferences.putString(ssidKey.c_str(), ssid);
        preferences.putString(passKey.c_str(), password);
        preferences.putInt("count", networkCount + 1);
        
        wifiMulti.addAP(ssid.c_str(), password.c_str());
        
        Serial.printf("✓ Nova mreža sačuvana: %s\n", ssid.c_str());
    } else {
        Serial.println("⚠️  Memorija puna! Obrišite neku mrežu prvo.");
    }
}

void removeNetwork(String ssid) {
    int networkCount = preferences.getInt("count", 0);
    
    for(int i = 0; i < networkCount; i++) {
        String ssidKey = "ssid" + String(i);
        String existingSsid = preferences.getString(ssidKey.c_str(), "");
        
        if(existingSsid == ssid) {
            // Pomjeri sve mreže nakon ovoga za jedno mjesto nazad
            for(int j = i; j < networkCount - 1; j++) {
                String srcSsidKey = "ssid" + String(j + 1);
                String srcPassKey = "pass" + String(j + 1);
                String dstSsidKey = "ssid" + String(j);
                String dstPassKey = "pass" + String(j);
                
                preferences.putString(dstSsidKey.c_str(), preferences.getString(srcSsidKey.c_str(), ""));
                preferences.putString(dstPassKey.c_str(), preferences.getString(srcPassKey.c_str(), ""));
            }
            
            // Obriši zadnju
            String lastSsidKey = "ssid" + String(networkCount - 1);
            String lastPassKey = "pass" + String(networkCount - 1);
            preferences.remove(lastSsidKey.c_str());
            preferences.remove(lastPassKey.c_str());
            
            preferences.putInt("count", networkCount - 1);
            
            Serial.printf("✓ Obrisana mreža: %s\n", ssid.c_str());
            return;
        }
    }
}

bool connectToKnownNetworks() {
    Serial.println("🔄 Pokušavam povezivanje...");
    
    // Pokušaj povezivanje 20 sekundi
    for(int i = 0; i < 40; i++) {
        if(wifiMulti.run() == WL_CONNECTED) {
            return true;
        }
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\n✗ Povezivanje neuspješno");
    return false;
}

void startAPMode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-Car-WiFi", "12345678");
    
    isAPMode = true;
    isConnectedToWiFi = false;
    
    Serial.println("\n═══════════════════════════════════════");
    Serial.println("   📡 AP MOD AKTIVIRAN");
    Serial.println("═══════════════════════════════════════");
    Serial.println("  WiFi: ESP32-Car-WiFi");
    Serial.println("  Pass: 12345678");
    Serial.print("  IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("  URL: http://192.168.4.1");
    Serial.println("═══════════════════════════════════════\n");
}

void updateLEDStatus() {
    static unsigned long lastBlink = 0;
    static bool ledState = false;
    
    unsigned long now = millis();
    
    if(isConnectedToWiFi && isWebSocketConnected) {
        // Stalno upaljen - sve radi
        digitalWrite(STATUS_LED, HIGH);
    } else if(isConnectedToWiFi && !isWebSocketConnected) {
        // Sporo blinkanje - WiFi OK, WebSocket down
        if(now - lastBlink > 1000) {
            lastBlink = now;
            ledState = !ledState;
            digitalWrite(STATUS_LED, ledState);
        }
    } else if(isAPMode) {
        // Brzo blinkanje - AP mod
        if(now - lastBlink > 200) {
            lastBlink = now;
            ledState = !ledState;
            digitalWrite(STATUS_LED, ledState);
        }
    } else {
        // Isključen - nije povezan
        digitalWrite(STATUS_LED, LOW);
    }
}

// ═══════════════════════════════════════════════════════════
// WEB SERVER HANDLERS
// ═══════════════════════════════════════════════════════════

void setupWebServer() {
    server.on("/", handleRoot);
    server.on("/scan", handleScan);
    server.on("/connect", HTTP_POST, handleConnect);
    server.on("/saved", handleSavedNetworks);
    server.on("/remove", HTTP_POST, handleRemoveNetwork);
    server.on("/status", handleStatus);
}

void handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="sr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Auto - WiFi Manager</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 15px;
        }
        .container { 
            max-width: 600px; 
            margin: 0 auto; 
            background: white; 
            border-radius: 20px; 
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 25px 20px;
            text-align: center;
        }
        .header h1 { font-size: 24px; margin-bottom: 5px; }
        .status-badge {
            display: inline-block;
            padding: 5px 15px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 600;
            margin-top: 10px;
        }
        .status-online { background: #4caf50; }
        .status-offline { background: #f44336; }
        .status-ap { background: #ff9800; }
        .content { padding: 20px; }
        .section {
            background: #f8f9fa;
            border-radius: 12px;
            padding: 20px;
            margin-bottom: 20px;
        }
        .section h2 { font-size: 18px; margin-bottom: 15px; color: #333; }
        .network-list {
            list-style: none;
        }
        .network-item {
            background: white;
            border: 1px solid #e0e0e0;
            border-radius: 10px;
            padding: 12px 15px;
            margin-bottom: 10px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            cursor: pointer;
            transition: all 0.3s;
        }
        .network-item:hover { 
            border-color: #667eea; 
            transform: translateX(5px);
        }
        .network-info { flex: 1; }
        .network-name { font-weight: 600; color: #333; }
        .network-signal { 
            font-size: 12px; 
            color: #666; 
            margin-top: 3px;
        }
        .signal-strong { color: #4caf50; }
        .signal-medium { color: #ff9800; }
        .signal-weak { color: #f44336; }
        .btn {
            padding: 10px 20px;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
        }
        .btn-primary {
            background: #667eea;
            color: white;
        }
        .btn-danger {
            background: #f44336;
            color: white;
        }
        .btn-refresh {
            width: 100%;
            background: #667eea;
            color: white;
            margin-bottom: 15px;
        }
        .btn:hover { opacity: 0.9; transform: translateY(-2px); }
        .input-group {
            margin-bottom: 15px;
        }
        .input-group label {
            display: block;
            margin-bottom: 5px;
            font-weight: 600;
            font-size: 14px;
            color: #555;
        }
        .input-group input {
            width: 100%;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 8px;
            font-size: 14px;
        }
        .saved-network {
            display: flex;
            justify-content: space-between;
            align-items: center;
            background: white;
            padding: 12px;
            border-radius: 8px;
            margin-bottom: 10px;
            border: 1px solid #e0e0e0;
        }
        .loader {
            border: 3px solid #f3f3f3;
            border-top: 3px solid #667eea;
            border-radius: 50%;
            width: 30px;
            height: 30px;
            animation: spin 1s linear infinite;
            margin: 20px auto;
        }
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
        #connectModal {
            display: none;
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0,0,0,0.5);
            justify-content: center;
            align-items: center;
            z-index: 1000;
        }
        .modal-content {
            background: white;
            border-radius: 15px;
            padding: 25px;
            max-width: 400px;
            width: 90%;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🚗 ESP32 Auto WiFi</h1>
            <div class="status-badge" id="statusBadge">Učitavam...</div>
        </div>
        
        <div class="content">
            <!-- Dostupne mreže -->
            <div class="section">
                <h2>📡 Dostupne WiFi Mreže</h2>
                <button class="btn btn-refresh" onclick="scanNetworks()">🔄 Skeniraj Mreže</button>
                <div id="scanResults">
                    <div class="loader"></div>
                    <p style="text-align:center; color:#666;">Skeniram...</p>
                </div>
            </div>
            
            <!-- Sačuvane mreže -->
            <div class="section">
                <h2>💾 Sačuvane Mreže</h2>
                <div id="savedNetworks"></div>
            </div>
        </div>
    </div>

    <!-- Modal za unos lozinke -->
    <div id="connectModal">
        <div class="modal-content">
            <h2 style="margin-bottom:20px;">🔐 Povezivanje</h2>
            <div class="input-group">
                <label>WiFi Mreža:</label>
                <input type="text" id="modalSSID" readonly>
            </div>
            <div class="input-group">
                <label>Lozinka:</label>
                <input type="password" id="modalPassword" placeholder="Unesite lozinku">
            </div>
            <button class="btn btn-primary" style="width:100%; margin-bottom:10px;" onclick="connectToNetwork()">
                Poveži se
            </button>
            <button class="btn" style="width:100%; background:#ddd;" onclick="closeModal()">
                Otkaži
            </button>
        </div>
    </div>

    <script>
        let currentSSID = '';
        
        // Load initial data
        window.onload = function() {
            updateStatus();
            loadSavedNetworks();
            scanNetworks();
        };
        
        function updateStatus() {
            fetch('/status')
                .then(r => r.json())
                .then(data => {
                    const badge = document.getElementById('statusBadge');
                    if(data.connected) {
                        badge.className = 'status-badge status-online';
                        badge.textContent = '✓ Povezan: ' + data.ssid;
                    } else if(data.apMode) {
                        badge.className = 'status-badge status-ap';
                        badge.textContent = '📡 AP Mod';
                    } else {
                        badge.className = 'status-badge status-offline';
                        badge.textContent = '✗ Nije povezan';
                    }
                });
        }
        
        function scanNetworks() {
            document.getElementById('scanResults').innerHTML = '<div class="loader"></div><p style="text-align:center; color:#666;">Skeniram...</p>';
            
            fetch('/scan')
                .then(r => r.json())
                .then(networks => {
                    let html = '<ul class="network-list">';
                    networks.forEach(net => {
                        let signalClass = net.rssi > -60 ? 'signal-strong' : (net.rssi > -75 ? 'signal-medium' : 'signal-weak');
                        let signalBars = net.rssi > -60 ? '▂▄▆█' : (net.rssi > -75 ? '▂▄▆' : '▂▄');
                        
                        html += `<li class="network-item" onclick="showConnectModal('${net.ssid}')">
                            <div class="network-info">
                                <div class="network-name">${net.ssid}</div>
                                <div class="network-signal ${signalClass}">${signalBars} ${net.rssi} dBm</div>
                            </div>
                            <span style="color:#667eea;">→</span>
                        </li>`;
                    });
                    html += '</ul>';
                    document.getElementById('scanResults').innerHTML = html;
                });
        }
        
        function loadSavedNetworks() {
            fetch('/saved')
                .then(r => r.json())
                .then(networks => {
                    let html = '';
                    if(networks.length === 0) {
                        html = '<p style="color:#999; text-align:center;">Nema sačuvanih mreža</p>';
                    } else {
                        networks.forEach(net => {
                            html += `<div class="saved-network">
                                <span><strong>${net}</strong></span>
                                <button class="btn btn-danger" onclick="removeNetwork('${net}')">Obriši</button>
                            </div>`;
                        });
                    }
                    document.getElementById('savedNetworks').innerHTML = html;
                });
        }
        
        function showConnectModal(ssid) {
            currentSSID = ssid;
            document.getElementById('modalSSID').value = ssid;
            document.getElementById('modalPassword').value = '';
            document.getElementById('connectModal').style.display = 'flex';
        }
        
        function closeModal() {
            document.getElementById('connectModal').style.display = 'none';
        }
        
        function connectToNetwork() {
            const password = document.getElementById('modalPassword').value;
            
            if(!password && !confirm('Povezivanje bez lozinke (otvorena mreža)?')) {
                return;
            }
            
            closeModal();
            alert('Povezivanje u toku... Auto će se restartovati ako je uspješno.');
            
            fetch('/connect', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: `ssid=${encodeURIComponent(currentSSID)}&password=${encodeURIComponent(password)}`
            }).then(() => {
                setTimeout(() => {
                    location.reload();
                }, 5000);
            });
        }
        
        function removeNetwork(ssid) {
            if(!confirm('Obrisati mrežu: ' + ssid + '?')) return;
            
            fetch('/remove', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: `ssid=${encodeURIComponent(ssid)}`
            }).then(() => {
                loadSavedNetworks();
            });
        }
        
        // Auto-refresh status svakih 10 sekundi
        setInterval(updateStatus, 10000);
    </script>
</body>
</html>
)rawliteral";
    
    server.send(200, "text/html", html);
}

void handleScan() {
    Serial.println("🔍 Web zahtjev: Skeniranje mreža...");
    
    int n = WiFi.scanNetworks();
    
    String json = "[";
    for (int i = 0; i < n; i++) {
        if(i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN) + "}";
    }
    json += "]";
    
    server.send(200, "application/json", json);
    WiFi.scanDelete();
}

void handleConnect() {
    if(!server.hasArg("ssid")) {
        server.send(400, "text/plain", "Missing SSID");
        return;
    }
    
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    Serial.printf("📡 Pokušavam povezivanje na: %s\n", ssid.c_str());
    
    // Sačuvaj mrežu
    saveNetwork(ssid, password);
    
    server.send(200, "text/plain", "Connecting...");
    
    delay(1000);
    
    // Resetuj i pokuša ponovo povezivanje
    ESP.restart();
}

void handleSavedNetworks() {
    int networkCount = preferences.getInt("count", 0);
    
    String json = "[";
    for(int i = 0; i < networkCount; i++) {
        String ssidKey = "ssid" + String(i);
        String ssid = preferences.getString(ssidKey.c_str(), "");
        
        if(ssid.length() > 0) {
            if(i > 0) json += ",";
            json += "\"" + ssid + "\"";
        }
    }
    json += "]";
    
    server.send(200, "application/json", json);
}

void handleRemoveNetwork() {
    if(!server.hasArg("ssid")) {
        server.send(400, "text/plain", "Missing SSID");
        return;
    }
    
    String ssid = server.arg("ssid");
    removeNetwork(ssid);
    
    server.send(200, "text/plain", "Removed");
}

void handleStatus() {
    JsonDocument doc;
    doc["connected"] = isConnectedToWiFi;
    doc["apMode"] = isAPMode;
    doc["ssid"] = isConnectedToWiFi ? WiFi.SSID() : "";
    doc["ip"] = isConnectedToWiFi ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    doc["rssi"] = isConnectedToWiFi ? WiFi.RSSI() : 0;
    
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

// ═══════════════════════════════════════════════════════════
// OSTALE FUNKCIJE (nepromijenjene)
// ═══════════════════════════════════════════════════════════

void calibrateAcsOffset() {
    Serial.println("Calibrating ACS712 offset...");
    long totalAdc = 0;
    for (int i = 0; i < 500; i++) {
        totalAdc += analogRead(acsPin);
        delay(2);
    }
    float avgAdc = totalAdc / 500.0;
    float voltageOnPin = avgAdc * (ADC_REFERENCE / adcResolution);
    acsOffset_mV = voltageOnPin * (R1_ACS + R2_ACS) / R2_ACS * 1000.0;
    Serial.printf("ACS712 calibration complete. Offset: %.2f mV\n", acsOffset_mV);
}

void calculateRpmAndSpeed() {
    int hallValue = analogRead(hallSensorPin);
    
    if (hallValue > hallThreshold && !magnetDetected) {
        magnetDetected = true;
        unsigned long now = millis();
        if (lastPulseTime > 0) {
            pulseInterval = now - lastPulseTime;
        }
        lastPulseTime = now;
    } else if (hallValue < hallThreshold - 100 && magnetDetected) {
        magnetDetected = false;
    }

    if (millis() - lastPulseTime > 1500) {
        currentRpm = 0;
        currentSpeed = 0;
        motorRpm = 0;
        pulseInterval = 0;
    } else if (pulseInterval > 0) {
        float wheelRpm = (60.0 * 1000.0) / pulseInterval / pulsesPerRevolution;
        motorRpm = wheelRpm * gearRatio;
        currentSpeed = (wheelCircumference * wheelRpm * 60) / 1000.0;
        currentRpm = wheelRpm;
    }
}

float readBatteryVoltage() {
    long totalAdc = 0;
    for (int i = 0; i < 10; i++) {
        totalAdc += analogRead(batteryPin);
        delay(1);
    }
    float avgAdc = totalAdc / 10.0;
    float voltageOnPin = avgAdc * (ADC_REFERENCE / adcResolution);
    return voltageOnPin * (R1_BAT + R2_BAT) / R2_BAT;
}

float readCurrent() {
    long totalAdc = 0;
    for (int i = 0; i < 100; i++) {
        totalAdc += analogRead(acsPin);
        delay(1);
    }
    float avgAdc = totalAdc / 100.0;
    float voltageOnPin = avgAdc * (ADC_REFERENCE / adcResolution);
    float sensorVoltage_mV = voltageOnPin * (R1_ACS + R2_ACS) / R2_ACS * 1000.0;
    float current_A = (sensorVoltage_mV - acsOffset_mV) / ACS_SENSITIVITY;
    return abs(current_A);
}

void sendTelemetry() {
    JsonDocument doc;
    batteryVoltage = readBatteryVoltage();
    current = readCurrent();
    batteryPercentage = ((batteryVoltage - minBatteryVoltage) / (maxBatteryVoltage - minBatteryVoltage)) * 100.0;
    batteryPercentage = constrain(batteryPercentage, 0, 100);
    battery_mAhRemaining = BATTERY_CAPACITY_mAh * batteryPercentage / 100.0;

    doc["BatteryLevel"] = round(batteryPercentage);
    doc["BatteryRemaining_mAh"] = round(battery_mAhRemaining);
    doc["CurrentConsumption"] = current;
    doc["Speed"] = currentSpeed;
    doc["MotorRpm"] = round(currentRpm);

    String telemetryJson;
    serializeJson(doc, telemetryJson);
    String message = "{\"type\":1,\"target\":\"SendTelemetryData\",\"arguments\":[" + telemetryJson + "]}\x1e";
    webSocket.sendTXT(message);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            {  // <-- DODAJTE vitičaste zagrade
                isWebSocketConnected = false;
                Serial.println("[WebSocket] Disconnected!");
            }  // <-- ZATVORITE zagrade
            break;
        
        case WStype_CONNECTED:
            {  // <-- DODAJTE vitičaste zagrade
                isWebSocketConnected = true;
                Serial.printf("[WebSocket] Connected: %s\n", payload);
                webSocket.sendTXT("{\"protocol\":\"json\",\"version\":1}\x1e");
                String msg = "{\"type\":1,\"target\":\"RegisterESP32\",\"arguments\":[\"" + String(deviceId) + "\"]}\x1e";
                webSocket.sendTXT(msg);
            }  // <-- ZATVORITE zagrade
            break;

        case WStype_TEXT:
            {  // <-- DODAJTE vitičaste zagrade
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, payload, length);
                
                if (!error && doc["type"] == 1 && strcmp(doc["target"], "ReceiveCommand") == 0) {
                    JsonObject cmd = doc["arguments"][0];
                    const char* cmdType = cmd["commandType"];
                    const char* val = cmd["value"];

                    if (strcmp(cmdType, "move") == 0) {
                        if (strcmp(val, "forward") == 0) goForward(200);
                        else if (strcmp(val, "backward") == 0) goBackward(150);
                        else if (strcmp(val, "left") == 0) turnLeft();
                        else if (strcmp(val, "right") == 0) turnRight();
                        else stopMotors();
                    } else if (strcmp(cmdType, "light") == 0) {
                        setLights(strcmp(val, "on") == 0);
                    }
                }
            }  // <-- ZATVORITE zagrade
            break;

        default:
            break;
    }
}


void goForward(int speed) {
    isReversing = false;
    digitalWrite(reverseLedPin, LOW);
    digitalWrite(motorIn1Pin, HIGH);
    digitalWrite(motorIn2Pin, LOW);
    ledcWrite(motorEnablePin, speed);
    currentPwmValue = speed;
}

void goBackward(int speed) {
    isReversing = true;
    digitalWrite(reverseLedPin, HIGH);
    digitalWrite(motorIn1Pin, LOW);
    digitalWrite(motorIn2Pin, HIGH);
    ledcWrite(motorEnablePin, speed);
    currentPwmValue = speed;
}

void stopMotors() {
    isReversing = false;
    digitalWrite(reverseLedPin, LOW);
    digitalWrite(motorIn1Pin, LOW);
    digitalWrite(motorIn2Pin, LOW);
    ledcWrite(motorEnablePin, 0);
    servo.write(90);
    currentPwmValue = 0;
}

void turnLeft() {
    servo.write(45);
}

void turnRight() {
    servo.write(135);
}

void setLights(bool on) {
    lightsOn = on;
    digitalWrite(frontLedPin, on);
}
