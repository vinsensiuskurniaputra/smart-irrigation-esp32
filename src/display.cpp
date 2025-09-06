#include "display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

// Definisi OLED
#define OLED_MOSI     23
#define OLED_CLK      18
#define OLED_DC       17
#define OLED_CS       5
#define OLED_RESET    16
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

extern String currentPlantType;
extern String actuatorMode; // "manual" | "auto"
extern bool lowMoistureAlert;
extern unsigned long lastBlinkTime;
extern bool blinkState;

// Objek Display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// Ikon (sama seperti sebelumnya, disingkat)
static const unsigned char PROGMEM plant_icon[] = { 0x00, 0x00, 0x38, 0x00 };
static const unsigned char PROGMEM temp_icon[]  = { 0x00, 0x00, 0x30, 0x00 };
static const unsigned char PROGMEM pump_icon[]  = { 0x00, 0x00, 0x00, 0x00 };
static const unsigned char PROGMEM alert_droplet_icon[] = { 0x00, 0x00, 0x80, 0x01 };

String capitalize(String str) {
    if (str.length() > 0) {
        str.toLowerCase();
        str[0] = toupper(str[0]);
    }
    return str;
}

void initDisplay() {
    if (!display.begin(SSD1306_SWITCHCAPVCC)) {
        Serial.println(F("Alokasi SSD1306 gagal"));
        for (;;);
    }

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 25);
    display.println("SIRAMIN");
    display.display();
    delay(2000);
}

void updateDisplay(float temp, float moisture, int threshold, const String& pumpStatusStr) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.drawBitmap(0, 0, plant_icon, 16, 16, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(18, 2);
    display.print(capitalize(currentPlantType));

    display.setTextSize(1);
    display.setCursor(88, 0);
    display.print(actuatorMode == "auto" ? "Auto" : "Manual");

    display.drawBitmap(0, 22, temp_icon, 16, 16, SSD1306_WHITE);
    display.setCursor(18, 26);
    display.printf("%.1f C", temp);

    display.drawBitmap(74, 22, pump_icon, 16, 16, SSD1306_WHITE);
    display.setCursor(92, 26);
    display.print(pumpStatusStr == "1" ? "ON" : "OFF");

    display.setCursor(0, 42);
    display.printf("Soil:%.0f%% Thr:%d%%", moisture, threshold);

    int barY = 53;
    int barHeight = 11;
    display.drawRect(0, barY, SCREEN_WIDTH, barHeight, SSD1306_WHITE);

    int fillWidth = map(moisture, 0, 100, 0, SCREEN_WIDTH - 2);
    display.fillRect(1, barY + 1, fillWidth, barHeight - 2, SSD1306_WHITE);

    int thresholdX = map(threshold, 0, 100, 0, SCREEN_WIDTH);
    display.drawFastVLine(thresholdX, barY, barHeight, SSD1306_INVERSE);

    if (lowMoistureAlert) {
        if (millis() - lastBlinkTime > 500) {
            lastBlinkTime = millis();
            blinkState = !blinkState;
        }
        if (blinkState) {
            display.drawBitmap(112, 0, alert_droplet_icon, 16, 16, SSD1306_WHITE);
        }
    }

    display.display();
}
