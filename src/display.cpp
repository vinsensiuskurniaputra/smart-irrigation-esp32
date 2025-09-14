#include "display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// Definisi OLED
#define OLED_RESET    -1
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

// Eksternal variabel dari file lain (misalnya main.cpp)
extern String currentPlantType;
extern String actuatorMode;
extern bool lowMoistureAlert;
extern unsigned long lastBlinkTime;
extern bool blinkState;

// Objek Display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Ikon-ikon Baru dan Ditingkatkan ---
static const unsigned char PROGMEM plant_leaf_icon[] = {
0x00, 0x00, 0x01, 0x80, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x1f, 0xf8, 0x3f, 0xfc, 0x7f, 0xfe,
0xff, 0xff, 0xff, 0xff, 0x80, 0x01, 0x80, 0x01, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x20, 0x04
};

static const unsigned char PROGMEM temp_thermometer_icon[] = {
0x00, 0x00, 0x07, 0x00, 0x18, 0x80, 0x18, 0x80, 0x20, 0x40, 0x20, 0x40, 0x20, 0x40, 0x20, 0x40,
0x10, 0x20, 0x10, 0x20, 0x1f, 0xe0, 0x1f, 0xe0, 0x1f, 0xe0, 0x1f, 0xe0, 0x1f, 0xe0, 0x1f, 0xe0
};

static const unsigned char PROGMEM pump_motor_icon[] = {
0x00, 0x00, 0x07, 0xe0, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x0f, 0xf0,
0x1f, 0xe0, 0x3e, 0x70, 0x7c, 0x30, 0xf8, 0x10, 0xf0, 0x10, 0xe0, 0x30, 0xc0, 0x70, 0x00, 0xf0
};

static const unsigned char PROGMEM alert_droplet_icon[] = {
0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x02, 0x40, 0x04, 0x20, 0x08, 0x10, 0x08, 0x10, 0x10, 0x08,
0x10, 0x08, 0x20, 0x04, 0x20, 0x04, 0x40, 0x02, 0x40, 0x02, 0x80, 0x01, 0x80, 0x01, 0x00, 0x00
};

// Fungsi utilitas untuk mengubah huruf pertama menjadi kapital
String capitalize(String str) {
    if (str.length() > 0) {
        str.toLowerCase();
        str[0] = toupper(str[0]);
    }
    return str;
}

// Fungsi inisialisasi display
void initDisplay() {
    Wire.begin(21, 22);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Alokasi SSD1306 gagal"));
        for (;;);
    }
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 25);
    display.println("AgriSense AI");
    display.display();
    delay(2000);
}

// Fungsi utama untuk memperbarui tampilan
void updateDisplay(float temp, float moisture, int threshold, const String& pumpStatusStr) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // --- Bagian Header ---
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("PLANT:");
    display.setTextSize(2);
    display.setCursor(45, 0);
    display.print(capitalize(currentPlantType));

    display.setTextSize(1);
    display.setCursor(0, 20);
    display.print("MODE:");
    display.setTextSize(1);
    display.setCursor(35, 20);
    display.print(actuatorMode == "auto" ? "Auto" : "Manual");

    // --- Bagian Utama: Suhu & Pompa ---
    display.drawBitmap(4, 38, temp_thermometer_icon, 16, 16, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(22, 38);
    display.printf("%.1fC", temp);

    display.drawBitmap(74, 38, pump_motor_icon, 16, 16, SSD1306_WHITE);
    display.setTextSize(2);
    // Menggunakan setCursor yang disesuaikan agar tulisan ON/OFF tidak terpotong
    display.setCursor(92, 38);
    display.print(pumpStatusStr == "1" ? "ON" : "OFF");

    // --- Bagian Bar Kelembaban & Peringatan ---
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.printf("Soil:%.0f%%", moisture);

    // Hitung posisi dan panjang bar kelembaban
    int barX = 60;
    int barY = 56;
    int barWidth = SCREEN_WIDTH - barX - 2;
    int barHeight = 8;
    display.drawRect(barX, barY, barWidth, barHeight, SSD1306_WHITE);

    int fillWidth = map(moisture, 0, 100, 0, barWidth - 2);
    display.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, SSD1306_WHITE);

    // Tampilkan ambang batas (threshold) di dalam bar
    int thresholdX = map(threshold, 0, 100, 0, barWidth);
    display.drawFastVLine(barX + thresholdX, barY, barHeight, SSD1306_INVERSE);

    // --- Peringatan Kedip (Blinking) ---
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