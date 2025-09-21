#include <Arduino.h>
#include <DHT.h>
#include "config.h"
#include "display.h"
#include "mqtt_handler.h"
#include "utils.h"

// Global variables (bisa dipakai di modul lain via extern)
String currentPlantType = "chili";
String actuatorMode = "manual";  // "manual" | "auto" - controlled via MQTT
String actuatorStatus = "off";    // "on" | "off" - controlled via MQTT subscription
bool lowMoistureAlert = false;
unsigned long lastBlinkTime = 0;
bool blinkState = false;

// DHT & sensor
DHT dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200);

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    analogReadResolution(12);
    dht.begin();

    initDisplay();
    setup_wifi();
}

void loop() {
    mqtt_loop();

    int sensorValue = analogRead(SOIL_PIN);
    float moisturePercent = map(sensorValue, 0, 4095, 100, 0);
    moisturePercent = constrain(moisturePercent, 0, 100);

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    if (isnan(temperature)) temperature = 0.0;
    if (isnan(humidity)) humidity = 0.0;

    // Use dynamic rule thresholds (received via MQTT) instead of static plant type function.
    extern int ruleMinMoisture;
    extern int ruleMaxMoisture;
    int threshold = ruleMinMoisture; // Use min as trigger threshold (pump turns on below this)
    bool pumpOn = false;

    // Auto mode: non-blocking state machine.
    // Behavior: when moisture < ruleMinMoisture start a cycle: run pump for 10s, then turn off and wait 60s.
    // If moisture goes above ruleMaxMoisture the cycle stops and pump stays off until re-triggered.
    const unsigned long PUMP_RUN_MS = 10UL * 1000UL;      // 10 seconds
    const unsigned long PUMP_COOLDOWN_MS = 60UL * 1000UL; // 60 seconds

    if (actuatorMode == "auto") {
        static uint8_t pumpState = 0; // 0=idle,1=running,2=cooldown
        static unsigned long stateStart = 0;
        unsigned long now = millis();

        // If soil sufficiently moist, reset to idle and stop the pump
        if (moisturePercent > ruleMaxMoisture) {
            pumpState = 0;
        }

        if (moisturePercent < ruleMinMoisture) {
            if (pumpState == 0) {
                // Start running immediately when first detected below threshold
                pumpState = 1;
                stateStart = now;
            } else if (pumpState == 1) {
                // Running: check run timeout
                if (now - stateStart >= PUMP_RUN_MS) {
                    pumpState = 2; // enter cooldown
                    stateStart = now;
                }
            } else if (pumpState == 2) {
                // Cooldown: wait then re-evaluate
                if (now - stateStart >= PUMP_COOLDOWN_MS) {
                    if (moisturePercent < ruleMinMoisture) {
                        pumpState = 1; // run again
                        stateStart = now;
                    } else {
                        pumpState = 0; // go idle
                    }
                }
            }
        } else {
            // Not below min threshold and not above max hysteresis: ensure pump is off
            if (pumpState != 0) pumpState = 0;
        }

        pumpOn = (pumpState == 1);
    } else { // manual
        pumpOn = (actuatorStatus == "on");
    }

    digitalWrite(RELAY_PIN, pumpOn ? HIGH : LOW);
    String pumpStatus = pumpOn ? "1" : "0";

    lowMoistureAlert = (moisturePercent < threshold);

    updateDisplay(temperature, moisturePercent, threshold, pumpStatus);
    mqtt_publishSensors(temperature, moisturePercent, humidity);
    mqtt_publishActualActuatorStatus(pumpStatus == "1");

    // Example rule publish (static for now - could be dynamic/config-driven)
    // Rule now subscribed; no longer publishing rule JSON here.

    Serial.printf("Plant:%s, Mode:%s, Moisture:%.1f%%, Temp:%.1fC, Hum:%.1f%%, Pump:%s, Thr:%d%%\n",
                  currentPlantType.c_str(),
                  (actuatorMode == "auto" ? "Auto" : "Manual"),
                  moisturePercent,
                  temperature,
                  humidity,
                  (pumpStatus == "1" ? "ON" : "OFF"),
                  threshold);


    delay(2000);
}
