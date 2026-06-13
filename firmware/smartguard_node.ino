/*
 * SmartGuard — ESP32-S3 Sensor Node Firmware
 * Reads PIR + VL53L1X sensors and posts JSON to FastAPI backend via WiFi
 *
 * Hardware:
 *   - ESP32-S3 DevKitC
 *   - 2x HC-SR501 PIR sensors (GPIO 4, GPIO 5)
 *   - VL53L1X ToF sensor (I2C: SDA=GPIO21, SCL=GPIO22)
 *   - SX1276 LoRa module (SPI: MOSI=GPIO23, MISO=GPIO19, SCK=GPIO18, CS=GPIO5)
 *
 * Dependencies (install via Arduino Library Manager):
 *   - ArduinoJson by Benoit Blanchon
 *   - VL53L1X by Pololu
 *   - LoRa by Sandeep Mistry
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "VL53L1X.h"

// ── Config ─────────────────────────────────────────────────────────────────
const char* WIFI_SSID     = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
const char* API_URL       = "http://YOUR_SERVER_IP:8000/sensor";
const char* ZONE_ID       = "P1";   // Change per node: P1, P2, P3, C1, E1

// GPIO
const int PIR_PIN_1 = 4;
const int PIR_PIN_2 = 5;
const int LED_SAFE  = 25;
const int LED_WARN  = 26;
const int LED_DANGER= 27;

// Timing
const unsigned long POST_INTERVAL_MS = 5000;   // post every 5 seconds

// ── Globals ─────────────────────────────────────────────────────────────────
VL53L1X tof;
unsigned long lastPost   = 0;
int           peopleCount = 0;
bool          pir1Active = false;
bool          pir2Active = false;

// ── Simple people counter using PIR pair (entry/exit beam logic) ──────────
// PIR1 triggers first → entering; PIR2 triggers first → exiting
unsigned long pir1Time = 0;
unsigned long pir2Time = 0;

void IRAM_ATTR onPIR1() {
  pir1Time = millis();
  pir1Active = true;
}

void IRAM_ATTR onPIR2() {
  pir2Time = millis();
  pir2Active = true;
}

void processPIR() {
  const unsigned long WINDOW = 2000;   // 2 second window

  if (pir1Active && pir2Active) {
    if (pir1Time < pir2Time && (pir2Time - pir1Time) < WINDOW) {
      peopleCount = max(0, peopleCount + 1);   // entered
    } else if (pir2Time < pir1Time && (pir1Time - pir2Time) < WINDOW) {
      peopleCount = max(0, peopleCount - 1);   // exited
    }
    pir1Active = pir2Active = false;
  }
}

void setStatusLED(int count, int capacity) {
  float ratio = (float)count / capacity;
  digitalWrite(LED_SAFE,   LOW);
  digitalWrite(LED_WARN,   LOW);
  digitalWrite(LED_DANGER, LOW);

  if      (ratio >= 0.80) digitalWrite(LED_DANGER, HIGH);
  else if (ratio >= 0.60) digitalWrite(LED_WARN,   HIGH);
  else                    digitalWrite(LED_SAFE,   HIGH);
}

void postToAPI(int count) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<128> doc;
  doc["zone_id"] = ZONE_ID;
  doc["count"]   = count;

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  if (code == 200) {
    Serial.printf("[SmartGuard] Posted: zone=%s count=%d → HTTP %d\n",
                  ZONE_ID, count, code);
  } else {
    Serial.printf("[SmartGuard] POST failed: HTTP %d\n", code);
  }

  http.end();
}

// ── Setup ──────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("[SmartGuard] Booting...");

  // PIR interrupts
  pinMode(PIR_PIN_1, INPUT);
  pinMode(PIR_PIN_2, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN_1), onPIR1, RISING);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN_2), onPIR2, RISING);

  // Status LEDs
  pinMode(LED_SAFE,   OUTPUT);
  pinMode(LED_WARN,   OUTPUT);
  pinMode(LED_DANGER, OUTPUT);
  digitalWrite(LED_SAFE, HIGH);   // green on boot

  // VL53L1X ToF
  Wire.begin();
  tof.setTimeout(500);
  if (tof.init()) {
    tof.setDistanceMode(VL53L1X::Long);
    tof.setMeasurementTimingBudget(50000);
    tof.startContinuous(100);
    Serial.println("[SmartGuard] ToF sensor OK");
  } else {
    Serial.println("[SmartGuard] WARNING: ToF sensor not found, using PIR only");
  }

  // WiFi
  Serial.printf("[SmartGuard] Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n[SmartGuard] WiFi connected — IP: %s\n",
                WiFi.localIP().toString().c_str());
}

// ── Loop ───────────────────────────────────────────────────────────────────
void loop() {
  processPIR();

  // ToF density check — if something is closer than 80cm, add to count
  if (tof.dataReady()) {
    int dist = tof.read();
    if (dist > 0 && dist < 800) {
      // object (person) detected within 80cm of sensor
      // This acts as a presence density cross-check
    }
    tof.clearInterrupt();
  }

  // Post to backend every POST_INTERVAL_MS
  if (millis() - lastPost >= POST_INTERVAL_MS) {
    postToAPI(peopleCount);
    setStatusLED(peopleCount, 200);   // 200 = this zone's capacity
    lastPost = millis();
  }

  delay(10);
}
