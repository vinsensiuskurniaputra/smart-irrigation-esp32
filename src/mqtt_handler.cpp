#include "mqtt_handler.h"
#include <WiFi.h>
#include <PubSubClient.h>

extern String currentPlantType;
extern String pumpMode;
extern String manualPumpStatus;

#define RELAY_PIN 5

const char* ssid = "CR HARYONO";
const char* password = "adiputwan03";
const char* mqtt_server = "192.168.1.8";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) msg += (char)payload[i];
    String topicStr = String(topic);

    Serial.printf("Pesan MQTT [%s]: %s\n", topic, msg.c_str());

    if (topicStr == "plant_type") currentPlantType = msg;
    else if (topicStr == "pump_mode") pumpMode = msg;
    else if (topicStr == "status") {
        manualPumpStatus = msg;
        if (pumpMode == "0") {
            digitalWrite(RELAY_PIN, manualPumpStatus == "1" ? HIGH : LOW);
        }
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
            client.subscribe("plant_type");
            client.subscribe("pump_mode");
            client.subscribe("status");
        } else {
            Serial.print("gagal, rc=");
            Serial.print(client.state());
            Serial.println(" coba lagi dalam 5 detik");
            delay(5000);
        }
    }
}

void mqtt_publishData(float temperature, float moisture, const String& pumpStatus, int threshold) {
    if (!client.connected()) return;
    client.publish("data_suhu", String(temperature, 1).c_str(), true);
    client.publish("kelembapan_tanah", String(moisture, 1).c_str(), true);
    client.publish("status", pumpStatus.c_str(), true);
    client.publish("threshold", String(threshold).c_str(), true);
}


void mqtt_loop() {
    if (!client.connected()) mqtt_reconnect();
    client.loop();
}
