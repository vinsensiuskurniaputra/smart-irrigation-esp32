#include <Arduino.h>
#include <DHT.h>
#include "display.h"
#include "mqtt_handler.h"
#include "utils.h"

// Global variables (bisa dipakai di modul lain via extern)
String currentPlantType = "chili";
String actuatorMode = "manual";  // "manual" | "auto"
String actuatorStatus = "off";    // desired state when manual
bool lowMoistureAlert = false;
unsigned long lastBlinkTime = 0;
bool blinkState = false;

// DHT & sensor
#define DHTPIN 22
#define DHTTYPE DHT22
#define SOIL_PIN 34
#define RELAY_PIN 33
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
    if (actuatorMode == "auto") {
        // Auto: turn on when below min threshold, turn off when above max threshold (hysteresis)
        extern int ruleMinMoisture;
        extern int ruleMaxMoisture;
        static bool lastState = false;
        if (moisturePercent < ruleMinMoisture) lastState = true;  // need water
        else if (moisturePercent > ruleMaxMoisture) lastState = false; // soil sufficiently moist
        pumpOn = lastState;
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
