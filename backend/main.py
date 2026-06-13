from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from typing import Optional
import sqlite3
import json
import time
import random
import math
from datetime import datetime, timedelta
from contextlib import asynccontextmanager
import asyncio
import threading

# ── DB Setup ─────────────────────────────────────────────────────────────────

DB_PATH = "smartguard.db"

def get_db():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    conn = get_db()
    conn.executescript("""
        CREATE TABLE IF NOT EXISTS sensor_readings (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            zone_id     TEXT    NOT NULL,
            count       INTEGER NOT NULL,
            timestamp   REAL    NOT NULL
        );

        CREATE TABLE IF NOT EXISTS alerts (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            zone_id     TEXT    NOT NULL,
            level       TEXT    NOT NULL,
            message     TEXT    NOT NULL,
            timestamp   REAL    NOT NULL,
            resolved    INTEGER DEFAULT 0
        );

        CREATE INDEX IF NOT EXISTS idx_sensor_zone_ts
            ON sensor_readings(zone_id, timestamp);
    """)
    conn.commit()
    conn.close()

# ── Zones Config ──────────────────────────────────────────────────────────────

ZONES = {
    "P1": {"name": "Platform 1 - Gate A",  "capacity": 200, "x": 120, "y": 160, "w": 180, "h": 60},
    "P2": {"name": "Platform 2 - Gate B",  "capacity": 180, "x": 120, "y": 260, "w": 180, "h": 60},
    "P3": {"name": "Platform 3 - Gate C",  "capacity": 220, "x": 120, "y": 360, "w": 180, "h": 60},
    "C1": {"name": "Central Concourse",    "capacity": 400, "x": 330, "y": 200, "w": 140, "h": 200},
    "E1": {"name": "East Exit / Ticketing","capacity": 150, "x": 510, "y": 240, "w": 130, "h": 120},
}

THRESHOLDS = {"safe": 0.60, "warning": 0.80}   # fraction of capacity

# ── Anomaly / Prediction (simple moving-average slope) ───────────────────────

def get_density_level(count: int, capacity: int) -> str:
    ratio = count / capacity
    if ratio >= THRESHOLDS["warning"]:
        return "danger"
    elif ratio >= THRESHOLDS["safe"]:
        return "warning"
    return "safe"

def predict_breach(zone_id: str, capacity: int) -> Optional[int]:
    """
    Returns estimated minutes until capacity breach, or None if not trending up.
    Uses a 5-reading linear regression slope.
    """
    conn = get_db()
    rows = conn.execute(
        "SELECT count, timestamp FROM sensor_readings "
        "WHERE zone_id=? ORDER BY timestamp DESC LIMIT 10",
        (zone_id,)
    ).fetchall()
    conn.close()

    if len(rows) < 3:
        return None

    counts = [r["count"] for r in reversed(rows)]
    times  = [r["timestamp"] for r in reversed(rows)]

    n = len(counts)
    t0 = times[0]
    xs = [(t - t0) for t in times]   # seconds since first reading

    # least-squares slope
    x_mean = sum(xs) / n
    y_mean = sum(counts) / n
    num   = sum((xs[i] - x_mean) * (counts[i] - y_mean) for i in range(n))
    denom = sum((xs[i] - x_mean) ** 2 for i in range(n)) or 1
    slope = num / denom   # people / second

    if slope <= 0:
        return None

    current = counts[-1]
    remaining = capacity * THRESHOLDS["warning"] - current
    if remaining <= 0:
        return 0

    seconds_to_breach = remaining / slope
    return max(0, int(seconds_to_breach / 60))   # convert to minutes

# ── Pydantic Models ───────────────────────────────────────────────────────────

class SensorPayload(BaseModel):
    zone_id: str
    count: int
    timestamp: Optional[float] = None

# ── App Bootstrap ─────────────────────────────────────────────────────────────

@asynccontextmanager
async def lifespan(app: FastAPI):
    init_db()
    # seed some historical data so graphs aren't empty on first load
    seed_historical_data()
    # start mock sensor simulator (comment out when using real hardware)
    t = threading.Thread(target=mock_sensor_loop, daemon=True)
    t.start()
    yield

app = FastAPI(title="SmartGuard API", version="1.0.0", lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# ── Mock Simulator (replace with real ESP32 data in production) ───────────────

_mock_state = {zid: int(cfg["capacity"] * 0.3) for zid, cfg in ZONES.items()}

def mock_sensor_loop():
    """Simulates realistic crowd dynamics — gradual build, sudden events."""
    while True:
        ts = time.time()
        conn = get_db()
        for zid, cfg in ZONES.items():
            cap = cfg["capacity"]
            current = _mock_state[zid]

            # simulate train arrival surge at C1 every ~2 minutes
            hour_fraction = (ts % 120) / 120
            surge = 30 * math.sin(math.pi * hour_fraction) if zid == "C1" else 0

            drift = random.gauss(0, 4) + surge
            new_count = max(5, min(cap + 20, int(current + drift)))
            _mock_state[zid] = new_count

            conn.execute(
                "INSERT INTO sensor_readings (zone_id, count, timestamp) VALUES (?,?,?)",
                (zid, new_count, ts)
            )

            # auto-alert
            level = get_density_level(new_count, cap)
            if level != "safe":
                msg = (
                    f"Zone {cfg['name']} at "
                    f"{int(new_count/cap*100)}% capacity — "
                    f"{'CRITICAL: immediate action required' if level=='danger' else 'Monitor closely'}."
                )
                conn.execute(
                    "INSERT INTO alerts (zone_id, level, message, timestamp) VALUES (?,?,?,?)",
                    (zid, level, msg, ts)
                )

        conn.commit()
        conn.close()
        time.sleep(5)   # 5-second polling interval

def seed_historical_data():
    conn = get_db()
    existing = conn.execute("SELECT COUNT(*) as c FROM sensor_readings").fetchone()["c"]
    if existing > 0:
        conn.close()
        return

    now = time.time()
    for zid, cfg in ZONES.items():
        cap = cfg["capacity"]
        base = int(cap * 0.25)
        for i in range(120):   # 2 hours of back-history at 1-min intervals
            ts = now - (120 - i) * 60
            # morning ramp-up pattern
            ramp = min(1.0, i / 60)
            count = int(base + cap * 0.4 * ramp + random.gauss(0, 8))
            count = max(5, min(cap, count))
            conn.execute(
                "INSERT INTO sensor_readings (zone_id, count, timestamp) VALUES (?,?,?)",
                (zid, count, ts)
            )

    conn.commit()
    conn.close()

# ── Routes ─────────────────────────────────────────────────────────────────────

@app.post("/sensor")
def receive_sensor_data(payload: SensorPayload):
    """Endpoint called by ESP32 nodes (or mock simulator)."""
    if payload.zone_id not in ZONES:
        raise HTTPException(status_code=400, detail="Unknown zone_id")

    ts = payload.timestamp or time.time()
    conn = get_db()
    conn.execute(
        "INSERT INTO sensor_readings (zone_id, count, timestamp) VALUES (?,?,?)",
        (payload.zone_id, payload.count, ts)
    )
    conn.commit()
    conn.close()
    return {"status": "ok"}


@app.get("/zones")
def get_zones_live():
    """Returns live crowd state for all zones — the dashboard's primary feed."""
    conn = get_db()
    result = {}

    for zid, cfg in ZONES.items():
        latest = conn.execute(
            "SELECT count, timestamp FROM sensor_readings "
            "WHERE zone_id=? ORDER BY timestamp DESC LIMIT 1",
            (zid,)
        ).fetchone()

        count    = latest["count"] if latest else 0
        cap      = cfg["capacity"]
        ratio    = round(count / cap, 3)
        level    = get_density_level(count, cap)
        eta      = predict_breach(zid, cap)

        # sparkline: last 20 readings
        spark = conn.execute(
            "SELECT count FROM sensor_readings "
            "WHERE zone_id=? ORDER BY timestamp DESC LIMIT 20",
            (zid,)
        ).fetchall()

        result[zid] = {
            **cfg,
            "count":    count,
            "ratio":    ratio,
            "level":    level,
            "eta_min":  eta,
            "spark":    [r["count"] for r in reversed(spark)],
        }

    conn.close()
    return result


@app.get("/history/{zone_id}")
def get_zone_history(zone_id: str, minutes: int = 60):
    """Time-series data for the prediction chart."""
    if zone_id not in ZONES:
        raise HTTPException(status_code=400, detail="Unknown zone_id")

    since = time.time() - minutes * 60
    conn  = get_db()
    rows  = conn.execute(
        "SELECT count, timestamp FROM sensor_readings "
        "WHERE zone_id=? AND timestamp>=? ORDER BY timestamp ASC",
        (zone_id, since)
    ).fetchall()
    conn.close()

    return [
        {
            "count": r["count"],
            "ts":    r["timestamp"],
            "label": datetime.fromtimestamp(r["timestamp"]).strftime("%H:%M"),
        }
        for r in rows
    ]


@app.get("/alerts")
def get_alerts(limit: int = 20):
    """Recent alert feed."""
    conn  = get_db()
    rows  = conn.execute(
        "SELECT * FROM alerts ORDER BY timestamp DESC LIMIT ?", (limit,)
    ).fetchall()
    conn.close()

    return [
        {
            "id":       r["id"],
            "zone_id":  r["zone_id"],
            "level":    r["level"],
            "message":  r["message"],
            "time":     datetime.fromtimestamp(r["timestamp"]).strftime("%H:%M:%S"),
            "resolved": bool(r["resolved"]),
        }
        for r in rows
    ]


@app.get("/stats/summary")
def get_summary():
    """Top-level KPIs for the header cards."""
    conn  = get_db()
    total_people = 0
    danger_zones = 0
    warning_zones = 0

    for zid, cfg in ZONES.items():
        row = conn.execute(
            "SELECT count FROM sensor_readings WHERE zone_id=? ORDER BY timestamp DESC LIMIT 1",
            (zid,)
        ).fetchone()
        if row:
            count = row["count"]
            total_people += count
            level = get_density_level(count, cfg["capacity"])
            if level == "danger":  danger_zones += 1
            elif level == "warning": warning_zones += 1

    alert_count = conn.execute(
        "SELECT COUNT(*) as c FROM alerts WHERE timestamp > ? AND resolved=0",
        (time.time() - 3600,)
    ).fetchone()["c"]

    conn.close()
    return {
        "total_people":  total_people,
        "danger_zones":  danger_zones,
        "warning_zones": warning_zones,
        "active_alerts": alert_count,
        "zones_monitored": len(ZONES),
    }
