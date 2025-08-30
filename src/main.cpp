#include <Arduino.h>
#include <DHT.h>
#include "display.h"
#include "mqtt_handler.h"
#include "utils.h"

// Global variables (bisa dipakai di modul lain via extern)
String currentPlantType = "chili";
String pumpMode = "0";
String manualPumpStatus = "0";
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
    if (isnan(temperature)) temperature = 0.0;

    int threshold = getThresholdByPlantType(currentPlantType);
    String pumpStatus = "0";

    if (pumpMode == "1") {
        if (moisturePercent < threshold) {
            digitalWrite(RELAY_PIN, HIGH);
            pumpStatus = "1";
        } else {
            digitalWrite(RELAY_PIN, LOW);
            pumpStatus = "0";
        }
    } else {
        pumpStatus = manualPumpStatus;
    }

    lowMoistureAlert = (moisturePercent < threshold);

    updateDisplay(temperature, moisturePercent, threshold, pumpStatus);
    mqtt_publishData(temperature, moisturePercent, pumpStatus, threshold);

    Serial.printf("Plant:%s, Mode:%s, Moisture:%.1f%%, Temp:%.1fC, Pump:%s, Thr:%d%%\n",
                  currentPlantType.c_str(),
                  (pumpMode == "1" ? "Auto" : "Manual"),
                  moisturePercent,
                  temperature,
                  (pumpStatus == "1" ? "ON" : "OFF"),
                  threshold);


    delay(2000);
}
