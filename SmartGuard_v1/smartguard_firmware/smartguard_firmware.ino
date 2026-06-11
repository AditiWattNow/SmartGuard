// ============================================
// SmartGuard v1 — Firmware
// ESP32-S3-WROOM-1
// Far Away Hackathon 2026
// Theme: Railways
// ============================================

#include <Wire.h>
#include <VL53L1X.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

// ============================================
// PIN DEFINITIONS
// ============================================

// I2C - VL53L1X
#define I2C_SDA       4
#define I2C_SCL       5
#define TOF_XSHUT     8

// SPI - RFM95W LoRa
#define LORA_MOSI     18
#define LORA_MISO     35
#define LORA_SCK      38
#define LORA_NSS      21
#define LORA_RST      41
#define LORA_DIO0     9

// PIR Sensors
#define PIR_A         6
#define PIR_B         7

// Status LEDs
#define LED_RED       46
#define LED_AMB       42
#define LED_GRN       47

// ============================================
// CONFIGURATION
// ============================================

#define LORA_FREQ         868E6    // 868 MHz India ISM band
#define NODE_ID           "PF_01"  // Platform node ID - change per node
#define SEND_INTERVAL     30000    // Send data every 30 seconds
#define DENSITY_THRESHOLD_AMBER  10  // People count for amber alert
#define DENSITY_THRESHOLD_RED    20  // People count for red alert

// VL53L1X distance thresholds (mm)
// Anything closer than this = person detected in doorway
#define PERSON_DISTANCE   1200     // 1.2 metres

// ============================================
// GLOBAL VARIABLES
// ============================================

VL53L1X tofSensor;

int peopleCount     = 0;    // Running total of people on platform
int peopleIn        = 0;    // People entered this interval
int peopleOut       = 0;    // People exited this interval
bool lastBeamState  = false;// Last state of ToF beam
bool pirAState      = false;
bool pirBState      = false;
unsigned long lastSendTime  = 0;
unsigned long lastTofRead   = 0;
String alertLevel   = "GREEN";

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  Serial.println("SmartGuard v1 — Booting...");

  // LED pins
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_AMB, OUTPUT);
  pinMode(LED_GRN, OUTPUT);

  // PIR pins
  pinMode(PIR_A, INPUT);
  pinMode(PIR_B, INPUT);

  // Boot indicator — flash all LEDs
  flashAllLEDs(3);

  // Initialise I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("I2C initialised");

  // Initialise VL53L1X
  pinMode(TOF_XSHUT, OUTPUT);
  digitalWrite(TOF_XSHUT, LOW);
  delay(10);
  digitalWrite(TOF_XSHUT, HIGH);
  delay(10);

  tofSensor.setTimeout(500);
  if (!tofSensor.init()) {
    Serial.println("ERROR: VL53L1X not found!");
    errorBlink(LED_RED);  // Fast red blink = sensor error
  } else {
    Serial.println("VL53L1X initialised");
    tofSensor.setDistanceMode(VL53L1X::Long);
    tofSensor.setMeasurementTimingBudget(50000);
    tofSensor.startContinuous(50);
  }

  // Initialise LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("ERROR: LoRa not found!");
    errorBlink(LED_AMB);  // Fast amber blink = LoRa error
  } else {
    Serial.println("LoRa initialised at 868 MHz");
    LoRa.setSpreadingFactor(9);       // SF9 - good range/speed balance
    LoRa.setSignalBandwidth(125E3);   // 125 kHz bandwidth
    LoRa.setCodingRate4(5);           // 4/5 coding rate
    LoRa.setTxPower(17);              // 17 dBm transmit power
  }

  // Ready — show green
  setLED("GREEN");
  Serial.println("SmartGuard ready!");
  Serial.print("Node ID: ");
  Serial.println(NODE_ID);
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  unsigned long now = millis();

  // Read ToF sensor every 100ms
  if (now - lastTofRead >= 100) {
    readToFSensor();
    lastTofRead = now;
  }

  // Read PIR sensors
  pirAState = digitalRead(PIR_A);
  pirBState = digitalRead(PIR_B);

  // Calculate density and update LEDs
  updateAlertLevel();

  // Send LoRa packet every 30 seconds
  if (now - lastSendTime >= SEND_INTERVAL) {
    sendLoRaPacket();
    lastSendTime = now;
  }
}

// ============================================
// TOF SENSOR — PEOPLE COUNTING
// ============================================

void readToFSensor() {
  if (!tofSensor.dataReady()) return;

  int distance = tofSensor.read();

  // Person detected if closer than threshold
  bool beamBroken = (distance < PERSON_DISTANCE);

  // Rising edge — person just walked into beam
  if (beamBroken && !lastBeamState) {
    // Simple direction detection using PIR
    // If PIR_A triggered recently = entering platform
    // If PIR_B triggered recently = exiting platform
    if (pirAState) {
      peopleCount++;
      peopleIn++;
      Serial.print("Person IN — Total: ");
      Serial.println(peopleCount);
    } else if (pirBState) {
      if (peopleCount > 0) peopleCount--;
      peopleOut++;
      Serial.print("Person OUT — Total: ");
      Serial.println(peopleCount);
    } else {
      // Default — assume entering
      peopleCount++;
      peopleIn++;
      Serial.print("Person detected — Total: ");
      Serial.println(peopleCount);
    }
  }

  lastBeamState = beamBroken;
}

// ============================================
// ALERT LEVEL + LED CONTROL
// ============================================

void updateAlertLevel() {
  if (peopleCount >= DENSITY_THRESHOLD_RED) {
    alertLevel = "RED";
    setLED("RED");
  } else if (peopleCount >= DENSITY_THRESHOLD_AMBER) {
    alertLevel = "AMBER";
    setLED("AMBER");
  } else {
    alertLevel = "GREEN";
    setLED("GREEN");
  }
}

void setLED(String level) {
  // Turn all off first
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_AMB, LOW);
  digitalWrite(LED_GRN, LOW);

  if (level == "RED")   digitalWrite(LED_RED, HIGH);
  if (level == "AMBER") digitalWrite(LED_AMB, HIGH);
  if (level == "GREEN") digitalWrite(LED_GRN, HIGH);
}

// ============================================
// LORA TRANSMISSION
// ============================================

void sendLoRaPacket() {
  // Build JSON payload
  StaticJsonDocument<256> doc;
  doc["node"]    = NODE_ID;
  doc["count"]   = peopleCount;
  doc["in"]      = peopleIn;
  doc["out"]     = peopleOut;
  doc["alert"]   = alertLevel;
  doc["pir_a"]   = pirAState;
  doc["pir_b"]   = pirBState;
  doc["uptime"]  = millis() / 1000;

  String payload;
  serializeJson(doc, payload);

  // Send via LoRa
  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();

  Serial.print("LoRa sent: ");
  Serial.println(payload);

  // Reset interval counters
  peopleIn  = 0;
  peopleOut = 0;
}

// ============================================
// UTILITY FUNCTIONS
// ============================================

void flashAllLEDs(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_AMB, HIGH);
    digitalWrite(LED_GRN, HIGH);
    delay(200);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_AMB, LOW);
    digitalWrite(LED_GRN, LOW);
    delay(200);
  }
}

void errorBlink(int pin) {
  // Fast blink forever to indicate error
  Serial.println("HALTED — Check sensor connection");
  while (true) {
    digitalWrite(pin, HIGH);
    delay(100);
    digitalWrite(pin, LOW);
    delay(100);
  }
}
