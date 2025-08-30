#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

void initDisplay();
void updateDisplay(float temp, float moisture, int threshold, const String& pumpStatusStr);

#endif
