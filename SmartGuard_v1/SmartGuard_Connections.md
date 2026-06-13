# SmartGuard v1 — PCB Connection Reference

## Project Overview
AI-powered railway platform crowd crush prevention system.
Custom PCB sensor node for real-time crowd density monitoring.

---

## Bill of Materials (BOM)

| Ref | Component | Package | Value | Source |
|-----|-----------|---------|-------|--------|
| U1 | ESP32-S3-WROOM-1 | SMD Castellated | Main MCU | Robu.in |
| U2 | VL53L1X Breakout | 6-pin header | ToF depth sensor | Thingbits.in |
| U3 | RFM95W-868S2 | SMD Module | LoRa 868MHz radio | Evelta.com |
| U4 | TP4056 Module | External module | LiPo charger 1A | Robu.in |
| U5 | LM7833 / AMS1117-3.3 | TO-220 THT | 3.3V LDO regulator | SP Road |
| R1 | Resistor | THT axial | 10kΩ | SP Road |
| R2 | Resistor | THT axial | 4.7kΩ | SP Road |
| R3 | Resistor | THT axial | 4.7kΩ | SP Road |
| R4 | Resistor | THT axial | 100Ω | SP Road |
| R5 | Resistor | THT axial | 1.2kΩ | SP Road |
| R6 | Resistor | THT axial | 100Ω | SP Road |
| R7 | Resistor | THT axial | 100Ω | SP Road |
| C1 | Ceramic Cap | THT disc | 100nF | SP Road |
| C2 | Ceramic Cap | THT disc | 100nF | SP Road |
| C3 | Ceramic Cap | THT disc | 100nF | SP Road |
| C4 | Electrolytic Cap | THT radial | 10µF | SP Road |
| C5 | Ceramic Cap | THT disc | 100nF | SP Road |
| D1 | LED Red | THT 5mm | Status indicator | SP Road |
| D2 | LED Amber | THT 5mm | Status indicator | SP Road |
| D3 | LED Green | THT 5mm | Status indicator | SP Road |
| J1 | PIR-A Connector | PinHeader 2.54mm 3-pin | HC-SR501 Zone A | SP Road |
| J2 | PIR-B Connector | PinHeader 2.54mm 3-pin | HC-SR501 Zone B | SP Road |
| J4 | LiPo Connector | JST-PH 2mm 2-pin | 3.7V 2000mAh battery | Robu.in |
| AE1 | Antenna | SMA EdgeMount | 868MHz wire antenna | SP Road |
| — | HC-SR501 × 2 | External module | PIR motion sensors | Robu.in |

**Estimated total cost: ₹2000–2500**

---

## Pin Assignments — ESP32-S3-WROOM-1

### I2C Bus (VL53L1X ToF sensor)
| ESP32-S3 Pin | Signal | Connected To |
|---|---|---|
| IO4 | I2C_SDA | VL53L1X SDA + 4.7kΩ pullup to 3V3 |
| IO5 | I2C_SCL | VL53L1X SCL + 4.7kΩ pullup to 3V3 |
| IO8 | TOF_XSHUT | VL53L1X XSHUT |

### SPI Bus (RFM95W LoRa)
| ESP32-S3 Pin | Signal | Connected To |
|---|---|---|
| IO18 | SPI_MOSI | RFM95W MOSI |
| IO35 | SPI_MISO | RFM95W MISO |
| IO38 | SPI_SCK | RFM95W SCK |
| IO21 | LORA_NSS | RFM95W NSS (chip select) |
| IO41 | LORA_RST | RFM95W RESET |
| IO9 | LORA_DIO0 | RFM95W DIO0 (interrupt) |

### GPIO (PIR sensors)
| ESP32-S3 Pin | Signal | Connected To |
|---|---|---|
| IO6 | PIR_A | HC-SR501 Zone A OUT |
| IO7 | PIR_B | HC-SR501 Zone B OUT |

### GPIO (Status LEDs)
| ESP32-S3 Pin | Signal | Connected To |
|---|---|---|
| IO46 | LED_RED | 100Ω resistor → Red LED anode |
| IO42 | LED_AMB | 100Ω resistor → Amber LED anode |
| IO47 | LED_GRN | 100Ω resistor → Green LED anode |

### Power
| ESP32-S3 Pin | Connected To |
|---|---|
| 3V3 | +3V3 power rail |
| GND | GND |
| EN | 10kΩ pullup to +3V3 |

---

## VL53L1X Breakout Board Connections

| VL53L1X Pin | Connected To |
|---|---|
| VDD | +3V3 |
| GND | GND |
| SDA | I2C_SDA (IO4) |
| SCL | I2C_SCL (IO5) |
| XSHUT | TOF_XSHUT (IO8) |
| GPIO1 | Not connected |

**I2C Address:** 0x29

---

## RFM95W-868S2 LoRa Module Connections

| RFM95W Pin | Connected To |
|---|---|
| 3.3V | +3V3 |
| GND | GND |
| MOSI | SPI_MOSI (IO18) |
| MISO | SPI_MISO (IO35) |
| SCK | SPI_SCK (IO38) |
| NSS | LORA_NSS (IO21) |
| RESET | LORA_RST (IO41) |
| DIO0 | LORA_DIO0 (IO9) |
| ANT | 8.2cm wire antenna (868MHz) |
| DIO1–DIO5 | Not connected |

**Frequency:** 868 MHz (India ISM band approved)
**Antenna:** 8.2cm straight wire for 868MHz λ/4

---

## HC-SR501 PIR Sensor Connections (×2)

| HC-SR501 Pin | Zone A | Zone B |
|---|---|---|
| VCC | +5V | +5V |
| GND | GND | GND |
| OUT | PIR_A (IO6) | PIR_B (IO7) |

**Note:** HC-SR501 OUT is 3.3V logic — safe for direct ESP32 GPIO connection.
**Sensitivity:** Adjust onboard potentiometer to ~3 metre range.
**Delay:** Set onboard delay pot to minimum (~5 seconds).

---

## Power Section

### TP4056 Charging Module
| TP4056 Module Pin | Connected To |
|---|---|
| USB input | External USB-C/Micro-USB (onboard) |
| B+ | LiPo battery positive (JST J4 pin 1) |
| B- | LiPo battery negative (JST J4 pin 2) / GND |
| OUT+ (VBAT) | LM7833 input (VI pin) |
| OUT- | GND |

**Charge current:** 1A (set by 1.2kΩ PROG resistor)

### LM7833 / AMS1117-3.3 Voltage Regulator (TO-220)
| LM7833 Pin | Connected To |
|---|---|
| VI (input) | VBAT from TP4056 OUT+ |
| VO (output) | +3V3 power rail |
| GND | GND |

**Input cap:** 10µF (C4) from VBAT to GND
**Output cap:** 100nF (C5) from +3V3 to GND

---

## Decoupling Capacitors

| Cap | Value | Placed Near | Between |
|---|---|---|---|
| C1 | 100nF | VL53L1X VDD pin | +3V3 and GND |
| C2 | 100nF | RFM95W 3.3V pin | +3V3 and GND |
| C3 | 100nF | TP4056 VCC pin | +5V and GND |
| C4 | 10µF | LM7833 input | VBAT and GND |
| C5 | 100nF | LM7833 output | +3V3 and GND |

---

## I2C Pull-up Resistors

| Resistor | Value | Connected Between |
|---|---|---|
| R2 | 4.7kΩ | I2C_SDA and +3V3 |
| R3 | 4.7kΩ | I2C_SCL and +3V3 |

---

## Status LED Circuit

| LED | Color | Resistor | GPIO |
|---|---|---|---|
| D1 | Red | R4 100Ω | IO46 |
| D2 | Amber | R6 100Ω | IO42 |
| D3 | Green | R7 100Ω | IO47 |

**Meaning:**
- 🟢 Green = Safe — crowd density normal
- 🟡 Amber = Warning — density rising, monitor
- 🔴 Red = Alert — critical density, trigger PA system

---

## Firmware Pin Definitions

```cpp
// ============================================
// SmartGuard v1 — Pin Definitions
// ESP32-S3-WROOM-1
// ============================================

// I2C — VL53L1X ToF sensor
#define I2C_SDA     4
#define I2C_SCL     5
#define TOF_XSHUT   8

// SPI — RFM95W LoRa
#define LORA_MOSI   18
#define LORA_MISO   35
#define LORA_SCK    38
#define LORA_NSS    21
#define LORA_RST    41
#define LORA_DIO0   9

// PIR sensors
#define PIR_A       6
#define PIR_B       7

// Status LEDs
#define LED_RED     46
#define LED_AMB     42
#define LED_GRN     47

// LoRa settings
#define LORA_FREQ   868E6   // 868 MHz India ISM band
```

---

## Power Rails Summary

| Rail | Voltage | Source | Powers |
|---|---|---|---|
| +5V | 5V | USB via TP4056 module | HC-SR501 PIR sensors |
| VBAT | 3.7V nominal | LiPo battery | LM7833 input |
| +3V3 | 3.3V | LM7833 regulator | ESP32-S3, VL53L1X, RFM95W |
| GND | 0V | Common ground | All components |

---

## PCB Design Rules Used

| Parameter | Value |
|---|---|
| Minimum track width | 0.2mm |
| Signal track width | 0.25mm |
| Power track width | 0.5mm |
| Minimum clearance | 0.2mm |
| Via diameter | 0.8mm |
| Via drill | 0.4mm |
| Board thickness | 1.6mm |
| Copper layers | 2 (F.Cu signals, B.Cu ground plane) |
| Board dimensions | ~100mm × 75mm |

---

## GitHub Repository Structure

```
SmartGuard_v1/
├── hardware/
│   ├── SmartGuard_v1.kicad_pro
│   ├── SmartGuard_v1.kicad_sch
│   ├── SmartGuard_v1.kicad_pcb
│   └── gerbers/
│       ├── SmartGuard_v1-F_Cu.gbr
│       ├── SmartGuard_v1-B_Cu.gbr
│       ├── SmartGuard_v1-F_Silkscreen.gbr
│       ├── SmartGuard_v1-Edge_Cuts.gbr
│       └── SmartGuard_v1-drill.drl
├── firmware/
│   └── smartguard_firmware/
│       ├── smartguard_firmware.ino
│       └── pins.h
├── software/
│   ├── backend/
│   └── dashboard/
├── docs/
│   ├── architecture_diagram.png
│   ├── schematic.pdf
│   └── BOM.csv
└── README.md
```

---

*SmartGuard v1 — Far Away Hackathon 2026*
*Theme: Railways — Build systems that make railways safer, smarter and more efficient*
