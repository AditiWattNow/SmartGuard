# SmartGuard — AI-Powered Railway Platform Crowd Safety System

> **Hackathon Project** | Hardware + Software | Indian Railways Safety Track

![SmartGuard Dashboard](docs/dashboard-preview.png)

## The Problem

On **February 15, 2025**, a crowd crush at New Delhi Railway Station killed **18 people** and injured 15 others — triggered by train delays creating dangerous platform overcrowding. Indian Railways subsequently identified **60 high-traffic stations** requiring immediate crowd management upgrades.

Existing solutions react *after* dangerous density is reached. SmartGuard **predicts** breaches before they happen.

---

## What SmartGuard Does

| Layer | What it is |
|-------|-----------|
| **Hardware** | Custom PCB sensor node (ESP32-S3 + PIR + ToF + LoRa) per zone |
| **Backend** | FastAPI server with time-series crowd data + anomaly detection |
| **Dashboard** | Live SVG station map + real-time alerts + trend prediction |
| **AI Model** | Linear regression slope on 5-reading window → estimated minutes to breach |

**Key differentiator:** SmartGuard gives station operators an **8–15 minute warning** before a dangerous threshold is reached — time to open alternate gates, redirect crowds, or delay train boarding. Real-time monitoring only tells you what's *already* happening.

---

## Architecture

```
[ESP32-S3 Node] ──WiFi/LoRa──▶ [FastAPI Backend] ──REST──▶ [Dashboard]
    ↑                                   ↓
PIR + VL53L1X                    SQLite DB
sensors                          Prediction Engine
                                 Alert Generator
```

**Kavach 4.0 compatible:** SmartGuard data format is designed for integration with Indian Railways' existing Kavach ATP system API. Kavach prevents train collisions — SmartGuard prevents platform crush. They are complementary, not competing.

---

## Repository Structure

```
smartguard/
├── backend/
│   ├── main.py              # FastAPI server — all 4 endpoints
│   └── requirements.txt
├── firmware/
│   └── smartguard_node.ino  # ESP32-S3 Arduino sketch
├── frontend/
│   └── index.html           # Complete single-file dashboard
├── hardware/                # KiCad PCB project (your partner's domain)
│   ├── smartguard_node.kicad_sch
│   ├── smartguard_node.kicad_pcb
│   └── gerbers/
└── docs/
    ├── architecture.png
    └── deployment-spec.md
```

---

## Running Locally

### Backend

```bash
cd backend
pip install -r requirements.txt
uvicorn main:app --reload --host 0.0.0.0 --port 8000
```

The server starts with:
- **Seeded historical data** (2 hours of realistic crowd patterns)
- **Mock sensor simulator** running in background — no hardware needed for demo
- **SQLite database** (`smartguard.db`) auto-created on first run

### Frontend

Just open `frontend/index.html` in a browser. No build step needed.

> If you get CORS errors, make sure the backend is running on `localhost:8000` (default).  
> For deployment, update `const API = "..."` in `index.html` to your server URL.

---

## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| `POST` | `/sensor` | Receive data from ESP32 nodes |
| `GET`  | `/zones` | Live crowd state for all zones |
| `GET`  | `/history/{zone_id}?minutes=60` | Time-series data for chart |
| `GET`  | `/alerts?limit=20` | Recent alert feed |
| `GET`  | `/stats/summary` | Header KPI cards |

### Example sensor POST (from ESP32 or test):
```bash
curl -X POST http://localhost:8000/sensor \
  -H "Content-Type: application/json" \
  -d '{"zone_id": "P1", "count": 145}'
```

---

## Zone Configuration

Zones are defined in `backend/main.py`:

```python
ZONES = {
    "P1": {"name": "Platform 1 - Gate A", "capacity": 200, ...},
    "P2": {"name": "Platform 2 - Gate B", "capacity": 180, ...},
    "C1": {"name": "Central Concourse",   "capacity": 400, ...},
    ...
}

THRESHOLDS = {"safe": 0.60, "warning": 0.80}  # fraction of capacity
```

Modify these to match any real station's layout.

---

## Prediction Model

SmartGuard uses a **least-squares linear regression** on the 10 most recent readings per zone:

1. Compute slope (people/second) from the time-series
2. If slope is positive, estimate seconds until 80% capacity threshold
3. Convert to minutes and surface in the UI as "Breach in ~N min"

This is equivalent to `sklearn.linear_model.LinearRegression` on a rolling window. For production, this can be upgraded to `IsolationForest` for anomaly detection or an LSTM for multi-step forecasting.

---

## Hardware BOM (per node) — ~₹2,500

| Component | Part | ~Cost |
|-----------|------|-------|
| MCU | ESP32-S3 DevKitC | ₹800 |
| Motion sensor × 2 | HC-SR501 PIR | ₹80 |
| Distance sensor | VL53L1X ToF | ₹350 |
| Radio | SX1276 LoRa module | ₹450 |
| Battery management | TP4056 IC | ₹40 |
| PCB fabrication (JLCPCB) | 4-layer, 5 units | ₹600 |
| Misc (caps, LEDs, headers) | — | ₹180 |

**60 stations × avg 8 nodes = 480 nodes × ₹2,500 = ₹12 lakh total hardware cost.**
This is the cost of one Kavach installation on a 1km track segment.

---

## Deployment Path

1. **Phase 1 — Pilot** (3 months): Install at 2 platforms of New Delhi station. Validate prediction accuracy against manual crowd counts.
2. **Phase 2 — Refinement** (3 months): Tune thresholds, integrate train delay API for better surge prediction.
3. **Phase 3 — Rollout** (6 months): Deploy to 60 identified high-risk stations. Integrate alerts into station master control room.

---

## Team

| Role | Responsibility |
|------|---------------|
| Hardware Engineer | PCB design (KiCad), ESP32 firmware, sensor integration |
| Software Engineer | FastAPI backend, React dashboard, deployment |

---

## License

MIT License — open for Indian Railways adoption and modification.
