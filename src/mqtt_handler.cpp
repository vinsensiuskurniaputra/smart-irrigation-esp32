#include "mqtt_handler.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

extern String currentPlantType;
extern String actuatorMode;    // "manual" | "auto"
extern String actuatorStatus;  // "on" | "off" (desired when manual)

// New device/topic constants
const char* DEVICE_CODE = "GH-001";
const int ACTUATOR_ID = 1; // pump

// Rule state variables (defaults)
int ruleMinMoisture = 40;
int ruleMaxMoisture = 80;
String rulePlantName = "Chili";
int rulePreferredHumidity = 70;
int rulePreferredTemp = 25;

// Helper to build topic strings
static String topicDeviceBase() { return String("device/") + DEVICE_CODE; }
static String topicActuatorBase() { return topicDeviceBase() + "/actuator/" + ACTUATOR_ID; }

const char* ssid = "CR HARYONO"; // TODO: move to config/secret store
const char* password = "adiputwan03"; // TODO: move to config/secret store
const char* mqtt_server = "202.10.48.12";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    String topicStr(topic);

    Serial.printf("MQTT IN [%s]: %s\n", topic, msg.c_str());

    // Topic patterns:
    // device/{device_code}/actuator/{actuator_id}/mode   value: manual|auto
    // device/{device_code}/actuator/{actuator_id}/status value: on|off (only applied in manual mode)
    // device/{device_code}/rule JSON rule object (SUBSCRIBE)
    String actuatorModeTopic = topicActuatorBase() + "/mode";
    String actuatorStatusTopic = topicActuatorBase() + "/status";
    String ruleTopic = topicDeviceBase() + "/rule";

    if (topicStr == actuatorModeTopic) {
        if (msg == "manual" || msg == "auto") actuatorMode = msg; else Serial.println("Unknown mode payload");
        return;
    }

    if (topicStr == actuatorStatusTopic) {
        // Parse JSON format: {"value":"on"} or {"value":"off"}
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, msg);
        if (err) {
            Serial.print("Status JSON parse error: ");
            Serial.println(err.c_str());
            return;
        }
        
        if (doc.containsKey("value")) {
            String status = String(doc["value"].as<const char*>());
            if (status == "on" || status == "off") {
                actuatorStatus = status;
                Serial.printf("Actuator status updated to: %s\n", actuatorStatus.c_str());
                if (actuatorMode == "manual") {
                    digitalWrite(RELAY_PIN, actuatorStatus == "on" ? HIGH : LOW);
                    Serial.printf("Relay set to: %s\n", actuatorStatus == "on" ? "HIGH" : "LOW");
                }
            } else {
                Serial.println("Unknown status value in JSON");
            }
        } else {
            Serial.println("Missing 'value' field in status JSON");
        }
        return;
    }

    if (topicStr == ruleTopic) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, msg);
        if (err) {
            Serial.print("Rule JSON parse error: ");
            Serial.println(err.c_str());
            return;
        }
        if (doc.containsKey("min_moisture")) ruleMinMoisture = doc["min_moisture"].as<int>();
        if (doc.containsKey("max_moisture")) ruleMaxMoisture = doc["max_moisture"].as<int>();
        if (doc.containsKey("plant_name")) rulePlantName = String(doc["plant_name"].as<const char*>());
        if (doc.containsKey("preferred_humidity")) rulePreferredHumidity = doc["preferred_humidity"].as<int>();
        if (doc.containsKey("preferred_temp")) rulePreferredTemp = doc["preferred_temp"].as<int>();
        Serial.printf("Updated rule: min=%d max=%d plant=%s prefHum=%d prefTemp=%d\n", ruleMinMoisture, ruleMaxMoisture, rulePlantName.c_str(), rulePreferredHumidity, rulePreferredTemp);
        return;
    }
}

void setup_wifi() {
    Serial.println("Menghubungkan ke WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    Serial.println("\nWiFi terhubung!");
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}


void mqtt_reconnect() {
    while (!client.connected()) {
        Serial.print("Mencoba koneksi MQTT...");
        // Tanpa username & password
        if (client.connect("ESP32_Plant_Monitor")) {
            Serial.println("terhubung");
            // Actuator & rule topics
            String modeTopic = topicActuatorBase() + "/mode";
            String statusTopic = topicActuatorBase() + "/status";
            String ruleTopic = topicDeviceBase() + "/rule";
            
            client.subscribe(modeTopic.c_str());
            client.subscribe(statusTopic.c_str());
            client.subscribe(ruleTopic.c_str());
            
            Serial.printf("Subscribed to topics:\n");
            Serial.printf("  Mode: %s\n", modeTopic.c_str());
            Serial.printf("  Status: %s\n", statusTopic.c_str());
            Serial.printf("  Rule: %s\n", ruleTopic.c_str());
        } else {
            Serial.print("gagal, rc=");
            Serial.print(client.state());
            Serial.println(" coba lagi dalam 5 detik");
            delay(5000);
        }
    }
}

// (Legacy publishing removed)

// New structured publish functions -----------------------------------------
void mqtt_publishSensors(float temperature, float moisture, float humidity) {
    if (!client.connected()) return;
    // sensor_id: soil moisture=1, temperature=2, humidity=3
    String base = topicDeviceBase() + "/sensor/";

    // Bungkus ke JSON
    String moistureJson   = "{\"value\":\"" + String(moisture, 1) + "\"}";
    String temperatureJson = "{\"value\":\"" + String(temperature, 1) + "\"}";
    String humidityJson    = "{\"value\":\"" + String(humidity, 1) + "\"}";

    client.publish(String(base + "1").c_str(), moistureJson.c_str(), true);
    client.publish(String(base + "2").c_str(), temperatureJson.c_str(), true);
    client.publish(String(base + "3").c_str(), humidityJson.c_str(), true);
}

void mqtt_publishActualActuatorStatus(bool isOn) {
    if (!client.connected()) return;
    String topic = topicActuatorBase() + "/actual-status";

    // Bungkus dalam JSON
    String payload = "{\"value\":\"" + String(isOn ? "on" : "off") + "\"}";

    client.publish(topic.c_str(), payload.c_str(), true);
}


void mqtt_loop() {
    if (!client.connected()) mqtt_reconnect();
    client.loop();
}
