#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>

void setup_wifi();
void mqtt_loop();
void mqtt_reconnect();
// Removed legacy publishData interface.

// New extended MQTT API
void mqtt_publishSensors(float temperature, float moisture, float humidity);
void mqtt_publishActualActuatorStatus(bool isOn);

// Constants for new topic schema
extern const char* DEVICE_CODE; // e.g., "GH-001"
extern const int ACTUATOR_ID;   // e.g., 1 (pump)

// Rule parameters updated via subscribed JSON on device/{device_code}/rule
extern int ruleMinMoisture;      // min_moisture
extern int ruleMaxMoisture;      // max_moisture
extern String rulePlantName;     // plant_name
extern int rulePreferredHumidity; // preferred_humidity
extern int rulePreferredTemp;     // preferred_temp

#endif
