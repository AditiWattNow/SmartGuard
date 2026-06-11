# SmartGuard — AI-Powered Railway Platform Crowd Crush Prevention

> **Far Away Hackathon 2026 — Railways Theme**
> *Build systems that make railways safer, smarter and more efficient.*

---

## The Problem

On February 2025, a crowd crush at New Delhi railway station killed 18 people and injured 15 others. The cause — dangerous overcrowding on a platform with no automated early warning system. Station staff had no way to know the density was reaching critical levels until it was too late.

**This is not an isolated incident.** India's 60 highest-traffic stations face this risk daily during peak hours and train delays.

---

## The Solution

SmartGuard is a low-cost, self-powered IoT sensor node that mounts on any platform pillar. It continuously counts people, detects dangerous density build-up, and alerts station staff **8–10 minutes before** a crush becomes inevitable — giving operators time to act.

No cameras. No WiFi dependency. No power cables. Just a small box on a pillar that could save lives.

---

## System Architecture

```
PIR Sensors + ToF Sensor
         ↓
   ESP32-S3 counts people
   and detects crowding
         ↓
   LEDs show alert level
   on the pillar instantly
         ↓
   LoRa radio sends data
   2km to control room
         ↓
   Dashboard shows live
   heatmap of all platforms
         ↓
   Auto alert to PA system
   "Please move to Platform 3"
```

---

## Hardware

### Custom PCB — SmartGuard v1

Designed in KiCad 9.0. 2-layer PCB with ground plane on B.Cu.

| Component | Function | Notes |
|---|---|---|
| ESP32-S3-WROOM-1 | Main MCU | WiFi + BLE + AI accelerator |
| VL53L1X ToF sensor | People counting | Laser beam counter at platform entrance |
| HC-SR501 × 2 | Motion detection | Zone A and Zone B PIR sensors |
| RFM95W-868S2 | LoRa radio | 868 MHz, approved India ISM band |
| TP4056 module | LiPo charging | USB-C input, 1A charge rate |
| LM7833 TO-220 | 3.3V regulation | Powers ESP32 + sensors |
| LEDs × 3 | Visual alert | Red / Amber / Green status |
| LiPo 3.7V 2000mAh | Power source | 8–12 hours per charge |

### Alert Thresholds

| LED | People Count | Meaning | Action |
|---|---|---|---|
| 🟢 Green | 0–9 | Safe | None |
| 🟡 Amber | 10–19 | Warning | Monitor closely |
| 🔴 Red | 20+ | Critical | Trigger PA, divert crowd |

### PCB Specifications

- Board size: ~100mm × 75mm
- Layers: 2 (F.Cu signals + B.Cu ground plane)
- Min track width: 0.25mm signal, 0.5mm power
- Designed for: JLCPCB fabrication
- Estimated unit cost: ₹2,000–2,500

---

## Pin Assignments — ESP32-S3

### I2C (VL53L1X)
| Pin | Signal |
|---|---|
| IO4 | SDA |
| IO5 | SCL |
| IO8 | XSHUT |

### SPI (RFM95W LoRa)
| Pin | Signal |
|---|---|
| IO18 | MOSI |
| IO35 | MISO |
| IO38 | SCK |
| IO21 | NSS |
| IO41 | RST |
| IO9 | DIO0 |

### GPIO
| Pin | Signal |
|---|---|
| IO6 | PIR Zone A |
| IO7 | PIR Zone B |
| IO46 | LED Red |
| IO42 | LED Amber |
| IO47 | LED Green |

---

## Firmware

Written for Arduino IDE with ESP32-S3 support.

### Required Libraries
- `VL53L1X` by Pololu
- `LoRa` by Sandeep Mistry
- `ArduinoJson` by Benoit Blanchon

### LoRa Packet Format (JSON)
```json
{
  "node":   "PF_01",
  "count":  14,
  "in":     3,
  "out":    1,
  "alert":  "AMBER",
  "pir_a":  true,
  "pir_b":  false,
  "uptime": 3600
}
```

### Board Settings (Arduino IDE)
- Board: `Arduino Nano ESP32`
- Upload Speed: `921600`
- USB Mode: `Hardware CDC and JTAG`

---

## Software (Backend + Dashboard)

- **FastAPI** Python backend receives LoRa packets
- **SQLite** stores time-series crowd data
- **Web dashboard** shows live platform heatmap
- **SMS alerts** via Twilio when density hits RED
- **Anomaly detection** using moving average — predicts dangerous density 8–10 minutes ahead

---

## Real-World Deployment

- **Cost per node:** ₹2,000–2,500
- **Battery life:** 8–12 hours (solar top-up available)
- **LoRa range:** up to 2km line of sight
- **Target deployment:** 60 high-risk stations identified by Indian Railways post-2025 crush
- **Kavach compatibility:** Exposes REST API compatible with Kavach 4.0 ATP system

---

## Repository Structure

```
SmartGuard/
├── hardware/
│   ├── SmartGuard_v1.kicad_pro
│   ├── SmartGuard_v1.kicad_sch
│   ├── SmartGuard_v1.kicad_pcb
│   └── gerbers/
│       ├── SmartGuard_v1-F_Cu.gtl
│       ├── SmartGuard_v1-B_Cu.gbl
│       ├── SmartGuard_v1-F_Mask.gts
│       ├── SmartGuard_v1-B_Mask.gbs
│       ├── SmartGuard_v1-F_Silkscreen.gto
│       ├── SmartGuard_v1-B_Silkscreen.gbo
│       ├── SmartGuard_v1-Edge_Cuts.gm1
│       ├── SmartGuard_v1-PTH.drl
│       └── SmartGuard_v1-NPTH.drl
├── firmware/
│   └── smartguard_firmware.ino
├── software/
│   ├── backend/
│   └── dashboard/
├── docs/
│   └── SmartGuard_Connections.md
└── README.md
```

---

## Team

**Aditi Samant** — Hardware, PCB Design, Firmware
**Vinayak Dadhwal** — Software, Backend, Dashboard

**Institution:** Army Institute of Technology, Pune
**City:** Pune, India

---

## Built With

![KiCad](https://img.shields.io/badge/KiCad-9.0-blue)
![ESP32](https://img.shields.io/badge/ESP32--S3-Arduino-red)
![LoRa](https://img.shields.io/badge/LoRa-868MHz-green)
![Python](https://img.shields.io/badge/Python-FastAPI-yellow)

---

*SmartGuard — Because 18 deaths at New Delhi station in 2025 should never happen again.*
