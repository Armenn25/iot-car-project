#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>

#define STATUS_LED 2

// --- WIFI ---
const char* ssid = "mojatv_full_0226";
const char* password = "123456123456";

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

// Filter za napon baterije
float smoothedBatteryVoltage = 0.0;
const float batterySmoothingFactor = 0.05;

// --- Control Variables ---
int currentPwmValue = 0;
bool isReversing = false;
bool lightsOn = false;

// --- Objects ---
WebSocketsClient webSocket;
Servo servo;
bool isWebSocketConnected = false;

// --- Function Prototypes ---
void calibrateAcsOffset();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void sendTelemetry();
void calculateRpmAndSpeed();
float readBatteryVoltage(bool initialize = false);  // ✅ ISPRAVLJENO
float readCurrent();
void goForward(int speed);
void goBackward(int speed);
void stopMotors();
void turnLeft();
void turnRight();
void setLights(bool on);

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
    
    // Inicijalizacija filtera
    smoothedBatteryVoltage = readBatteryVoltage(true); 
    
    calibrateAcsOffset();

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nConnected to WiFi");

    webSocket.beginSSL(websocket_server_host, websocket_server_port, websocket_server_path);
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);

    Serial.println("ESP32 Auto started!");
}

void loop() {
    webSocket.loop();
    calculateRpmAndSpeed();

    if (isWebSocketConnected && (millis() - lastTelemetrySend > telemetryInterval)) {
        lastTelemetrySend = millis();
        sendTelemetry();
    }

    digitalWrite(STATUS_LED, WiFi.status() == WL_CONNECTED && isWebSocketConnected ? HIGH : (millis() % 1000 < 500));
}

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
    
    Serial.printf("ACS712 offset: %.2f mV\n", acsOffset_mV);
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

float readBatteryVoltage(bool initialize) {
    long totalAdc = 0;
    for (int i = 0; i < 10; i++) {
        totalAdc += analogRead(batteryPin);
        delay(1);
    }
    float avgAdc = totalAdc / 10.0;
    float voltageOnPin = avgAdc * (ADC_REFERENCE / adcResolution);
    float rawVoltage = voltageOnPin * (R1_BAT + R2_BAT) / R2_BAT;

    if (initialize) {
        return rawVoltage;
    }

    // Low-pass filter
    smoothedBatteryVoltage = (batterySmoothingFactor * rawVoltage) + ((1.0 - batterySmoothingFactor) * smoothedBatteryVoltage);
    
    return smoothedBatteryVoltage;
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
            {
                isWebSocketConnected = false;
                Serial.println("[WebSocket] Disconnected!");
            }
            break;

        case WStype_CONNECTED:
            {
                isWebSocketConnected = true;
                Serial.printf("[WebSocket] Connected: %s\n", payload);
                webSocket.sendTXT("{\"protocol\":\"json\",\"version\":1}\x1e");
                String msg = "{\"type\":1,\"target\":\"RegisterESP32\",\"arguments\":[\"" + String(deviceId) + "\"]}\x1e";
                webSocket.sendTXT(msg);
            }
            break;

        case WStype_TEXT:
            {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, payload, length);
                
                if (error) {
                    Serial.print("JSON parse failed: ");
                    Serial.println(error.c_str());
                    return;
                }

                if (doc["type"] == 1 && strcmp(doc["target"], "ReceiveCommand") == 0) {
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
            }
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
