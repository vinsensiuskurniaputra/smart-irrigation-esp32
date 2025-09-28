#include "display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "mqtt_handler.h"

// Definisi OLED
#define OLED_RESET    -1
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

// Eksternal variabel dari modul lain
extern String rulePlantName;
extern String actuatorMode;
extern bool lowMoistureAlert;
extern unsigned long lastBlinkTime;
extern bool blinkState;
// Tambahkan variabel eksternal untuk humidity dari sensor DHT
extern float lastHumidity; 

// Manajemen Scene
static int currentScene = 0; // Mulai dari scene utama (0)
static unsigned long lastSceneChange = 0;
static const unsigned long SCENE_DURATION = 7000; // Durasi per scene 7 detik
static const int TOTAL_SCENES = 4; // Total 4 scene (0: Utama, 1: Info, 2: Lingkungan, 3: Tanah)

// Objek Display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Kumpulan Ikon ---
// Ikon Daun Tanaman (16x16)
static const unsigned char PROGMEM plant_leaf_icon[] = {
0x01, 0x80, 0x03, 0xc0, 0x06, 0x60, 0x0c, 0x30, 0x1f, 0xf8, 0x1f, 0xf8, 0x3f, 0xfc, 0x7f, 0xfe, 
0x7f, 0xfe, 0x7f, 0xfe, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x60, 0x06, 0x38, 0x1c
};

// Ikon Termometer (16x16)
static const unsigned char PROGMEM temp_thermometer_icon[] = {
0x07, 0x00, 0x08, 0x80, 0x08, 0x80, 0x10, 0x40, 0x10, 0x40, 0x10, 0x40, 0x10, 0x40, 0x08, 0x20, 
0x08, 0x20, 0x0f, 0xe0, 0x0f, 0xe0, 0x0f, 0xe0, 0x0f, 0xe0, 0x0f, 0xe0, 0x0f, 0xe0, 0x00, 0x00
};

// Ikon Pompa Air (16x16)
static const unsigned char PROGMEM pump_motor_icon[] = {
0x00, 0x00, 0x07, 0xe0, 0x08, 0x10, 0x78, 0x1c, 0x88, 0x12, 0x88, 0x12, 0x88, 0x12, 0x70, 0x0e, 
0x1f, 0xf8, 0x1f, 0xf8, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00
};

// Ikon Peringatan Kelembapan Rendah (16x16)
static const unsigned char PROGMEM alert_droplet_icon[] = {
0x01, 0x80, 0x01, 0x80, 0x02, 0x40, 0x04, 0x20, 0x08, 0x10, 0x10, 0x08, 0x10, 0x08, 0x20, 0x04, 
0x20, 0x04, 0x40, 0x02, 0x40, 0x02, 0x80, 0x01, 0x80, 0x01, 0x7f, 0xfe, 0x3f, 0xfc, 0x00, 0x00
};

// Ikon Tetesan Air untuk Kelembapan Udara (16x16)
static const unsigned char PROGMEM humidity_droplet_icon[] = {
0x00, 0x00, 0x01, 0x80, 0x03, 0xC0, 0x07, 0xE0, 0x0F, 0xF0, 0x1F, 0xF8, 0x3F, 0xFC, 0x3F, 0xFC,
0x1F, 0xF8, 0x0F, 0xF0, 0x07, 0xE0, 0x03, 0xC0, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Fungsi utilitas untuk mengubah huruf pertama menjadi kapital
String capitalize(String str) {
    if (str.length() > 0) {
        str.toLowerCase();
        str[0] = toupper(str[0]);
    }
    return str;
}

// Inisialisasi display
void initDisplay() {
    // Gunakan pin I2C default untuk ESP32 (GPIO 21, 22)
    Wire.begin(); 
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Alokasi SSD1306 gagal"));
        for (;;);
    }
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 25);
    display.println("AgriSense");
    display.display();
    delay(2000);
}

// --- FUNGSI-FUNGSI TAMPILAN SCENE ---

// Scene 0: Tampilan Utama (Dashboard)
void showMainScene(float temp, float moisture, int threshold, const String& pumpStatusStr) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // --- Baris 1: Nama Tanaman ---
    display.drawBitmap(0, 0, plant_leaf_icon, 16, 16, SSD1306_WHITE);
    String plantName = capitalize(rulePlantName);
    
    // Solusi: Ukuran Font Dinamis
    if (plantName.length() > 8) {
        display.setTextSize(1);
        display.setCursor(20, 4); // Sesuaikan posisi vertikal untuk font kecil
    } else {
        display.setTextSize(2);
        display.setCursor(20, 0);
    }
    display.print(plantName);

    // --- Baris 2: Data Sensor dan Status Pompa ---
    display.drawBitmap(0, 24, temp_thermometer_icon, 16, 16, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(20, 24);
    // **DIUBAH DI SINI: Menghilangkan koma pada suhu**
    display.printf("%.0fC", temp); 

    display.drawBitmap(72, 24, pump_motor_icon, 16, 16, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(92, 24);
    display.print(pumpStatusStr == "1" ? "ON" : "OFF");

    // --- Baris 3: Kelembapan Tanah (Tata Letak Baru) ---
    // 1. Tampilkan teks label di baris atasnya
    display.setTextSize(1);
    display.setCursor(0, 48); // Pindahkan teks ke y=48
    display.printf("Kelembapan Tanah: %.0f%%", moisture);

    // 2. Gambar progress bar di baris paling bawah (lebar penuh)
    int barX = 0;
    int barY = 58; // Turunkan posisi bar sedikit
    int barWidth = SCREEN_WIDTH; // Gunakan lebar penuh layar
    int barHeight = 6; // Buat bar sedikit lebih ramping
    display.drawRect(barX, barY, barWidth, barHeight, SSD1306_WHITE);

    // Mengisi bar sesuai nilai kelembapan
    int fillWidth = map(moisture, 0, 100, 0, barWidth - 2);
    display.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, SSD1306_WHITE);
    
    // Menampilkan garis ambang batas (threshold)
    int thresholdX = map(threshold, 0, 100, 0, barWidth - 2);
    display.drawFastVLine(barX + 1 + thresholdX, barY + 1, barHeight - 2, SSD1306_INVERSE);

    // --- Peringatan Kedip (Blinking Alert) ---
    if (lowMoistureAlert) {
        if (millis() - lastBlinkTime > 500) {
            lastBlinkTime = millis();
            blinkState = !blinkState;
        }
        if (blinkState) {
            display.drawBitmap(112, 0, alert_droplet_icon, 16, 16, SSD1306_WHITE);
        }
    }
}

// Scene 1: Info Konfigurasi Tanaman
void showPlantInfoScene(int threshold) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // Header
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("PLANT CONFIGURATION");
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);

    // Nama Tanaman (diberi ruang khusus agar tidak terpotong)
    display.setCursor(0, 18);
    display.print("Plant Name:");
    display.setTextSize(2);
    
    String plantName = capitalize(rulePlantName);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(plantName, 0, 0, &x1, &y1, &w, &h);
    // Tampilkan nama di tengah layar
    display.setCursor((SCREEN_WIDTH - w) / 2, 28);
    display.println(plantName);
    
    // Info Tambahan
    display.setTextSize(1);
    display.setCursor(0, 50);
    display.printf("Threshold: %d%% | Mode: %s", threshold, (actuatorMode == "auto" ? "Auto" : "Manual"));
}

// Scene 2: Data Sensor Lingkungan (DHT)
void showEnvironmentScene(float temperature, float humidity) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // Header
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("ENVIRONMENT SENSORS");
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    
    // Suhu
    display.drawBitmap(10, 20, temp_thermometer_icon, 16, 16, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(32, 20);
    display.printf("%.1f C", temperature);
    
    // Kelembapan Udara
    display.drawBitmap(10, 44, humidity_droplet_icon, 16, 16, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(32, 44);
    display.printf("%.1f %%", humidity);
}

// Scene 3: Detail Kelembapan Tanah
void showMoistureScene(float moisture, int threshold) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    // Header
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("SOIL MOISTURE");
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);

    // Angka Persentase Besar
    display.setTextSize(3);
    String moistureText = String((int)moisture) + "%";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(moistureText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 20);
    display.print(moistureText);

    // Bar Kelembapan di bagian bawah
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.printf("Threshold: %d%%", threshold);

    int barX = 0;
    int barY = 46;
    int barWidth = SCREEN_WIDTH;
    int barHeight = 8;
    display.drawRect(barX, barY, barWidth, barHeight, SSD1306_WHITE);

    int fillWidth = map(moisture, 0, 100, 0, barWidth - 2);
    display.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, SSD1306_WHITE);
    
    int thresholdX = map(threshold, 0, 100, 0, barWidth - 2);
    display.drawFastVLine(barX + 1 + thresholdX, barY + 1, barHeight - 2, SSD1306_INVERSE);
}

// Fungsi utama untuk manajemen dan pembaruan scene
void updateDisplay(float temp, float moisture, int threshold, const String& pumpStatusStr) {
    unsigned long currentTime = millis();
    
    // Ganti scene jika sudah waktunya
    if (currentTime - lastSceneChange >= SCENE_DURATION) {
        currentScene = (currentScene + 1) % TOTAL_SCENES; // Loop dari 0 ke 3
        lastSceneChange = currentTime;
    }
    
    // Tampilkan scene yang sedang aktif
    switch (currentScene) {
        case 0:
            showMainScene(temp, moisture, threshold, pumpStatusStr);
            break;
        case 1:
            showPlantInfoScene(threshold);
            break;
        case 2:
            // Pastikan Anda meneruskan nilai humidity dari sensor DHT di sini
            showEnvironmentScene(temp, lastHumidity); 
            break;
        case 3:
            showMoistureScene(moisture, threshold);
            break;
        default:
            showMainScene(temp, moisture, threshold, pumpStatusStr);
            break;
    }

    display.display();
}

// Fungsi updateDisplayScenes dengan parameter humidity tambahan
void updateDisplayScenes(float temp, float moisture, int threshold, float humidity) {
    // Update variabel global lastHumidity dengan nilai yang diterima
    lastHumidity = humidity;
    
    // Tentukan status pompa berdasarkan mode dan kondisi
    String pumpStatusStr = "0"; // Default OFF
    if (actuatorMode == "auto") {
        // Untuk auto mode, kita perlu menentukan status pompa berdasarkan kondisi
        // Ini adalah logika sederhana - bisa disesuaikan dengan kebutuhan
        pumpStatusStr = (moisture < threshold) ? "1" : "0";
    } else {
        // Untuk manual mode, gunakan actuatorStatus dari main.cpp
        extern String actuatorStatus;
        pumpStatusStr = (actuatorStatus == "on") ? "1" : "0";
    }
    
    // Panggil fungsi updateDisplay yang sudah ada
    updateDisplay(temp, moisture, threshold, pumpStatusStr);
}

// Fungsi untuk reset scene (opsional)
void resetSceneTiming() {
    lastSceneChange = millis();
    currentScene = 0;
}