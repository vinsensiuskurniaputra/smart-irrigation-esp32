#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>

void setup_wifi();
void mqtt_loop();
void mqtt_reconnect();
void mqtt_publishData(float temperature, float moisture, const String& pumpStatus, int threshold);

#endif
