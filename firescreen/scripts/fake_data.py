#!/usr/bin/env python3
"""Send fake dashboard data to the device over USB to exercise rendering,
especially label/number sizing edge cases.

    python3 scripts/fake_data.py --list
    python3 scripts/fake_data.py hazardous
    python3 scripts/fake_data.py all            # step through every scenario
    python3 scripts/fake_data.py '{"aq":{...}}' # raw JSON payload

Requires pyserial (provided by `nix develop`).

How it works: the device deep-sleeps between refreshes and its native USB
port only exists while it is awake. If the port is present, this script
resets the board via the USB control lines (same trick as esptool) to hit
the firmware's boot listening window; if not, it asks you to press the
RESET button and grabs the port as soon as it enumerates. Payloads that
arrive after the boot window are still processed: the firmware stays awake
and interactive for as long as a host holds the port open. Interactive
commands afterwards: paste JSON to render it, 'full' forces a full
refresh, 'exit'/'sleep' puts the device back to sleep.

Payload schema (all sections optional; a missing section renders dashes):
{
  "weather": {"temp": 82, "hi": 90, "lo": 70, "code": 0, "isDay": true,
              "wind": 3, "windDir": 45, "vis": 10,
              "days": [{"name": "Sat", "code": 1, "hi": 92, "lo": 67,
                        "wind": 3, "windDir": 45, "aqi": 42}, ...]},
  "aq": {"aqi": 302, "pm25": 402, "pm10": 309},
  "gov": 311,
  "history": {"range7d": false, "values": [aqi per bucket, -1 = gap]},
  "battery": 87,
  "now": <epoch seconds>
}
"""

import argparse
import glob
import json
import math
import sys
import time

import serial

# Must match CHART_BUCKETS_24H / CHART_BUCKETS_7D in include/config.h.
BUCKETS_24H = 44
BUCKETS_7D = 52


def wave(n, lo, hi, gaps=()):
    vals = []
    for i in range(n):
        v = lo + (hi - lo) * (0.5 + 0.5 * math.sin(i / n * math.tau * 1.5))
        vals.append(-1 if i in gaps else round(v))
    return vals


def weather(temp=82, hi=90, lo=70, code=0, wind=3, wind_dir=45, vis=10,
            aqi1=42, aqi2=57):
    return {
        "temp": temp, "hi": hi, "lo": lo, "code": code, "isDay": True,
        "wind": wind, "windDir": wind_dir, "vis": vis,
        "days": [
            {"name": "Sat", "code": 1, "hi": hi + 2, "lo": lo - 3,
             "wind": wind, "windDir": wind_dir, "aqi": aqi1},
            {"name": "Sun", "code": 3, "hi": hi - 2, "lo": lo - 7,
             "wind": wind * 2, "windDir": (wind_dir + 180) % 360,
             "aqi": aqi2},
        ],
    }


NOW = int(time.time())

SCENARIOS = {
    "normal": {
        "weather": weather(),
        "aq": {"aqi": 34, "pm25": 34, "pm10": 12},
        "gov": 28,
        "history": {"range7d": False, "values": wave(BUCKETS_24H, 15, 55)},
        "battery": 87,
        "now": NOW,
    },
    "hazardous": {  # 3-digit everything + longest short label
        "weather": weather(temp=102, hi=110, lo=68, code=95, wind=23,
                           wind_dir=225, vis=0.8, aqi1=412, aqi2=388),
        "aq": {"aqi": 402, "pm25": 402, "pm10": 309},
        "gov": 311,
        "history": {"range7d": False, "values": wave(BUCKETS_24H, 250, 460)},
        "battery": 12,
        "now": NOW,
    },
    "very-unhealthy": {  # longest AQI category label
        "weather": weather(temp=95, hi=99, lo=71, code=2),
        "aq": {"aqi": 258, "pm25": 258, "pm10": 145},
        "gov": 201,
        "history": {"range7d": False, "values": wave(BUCKETS_24H, 120, 290)},
        "battery": 54,
        "now": NOW,
    },
    "freezing": {  # negative temperatures, snow
        "weather": weather(temp=-12, hi=-5, lo=-22, code=73, wind=31,
                           wind_dir=0),
        "aq": {"aqi": 51, "pm25": 51, "pm10": 22},
        "gov": 48,
        "history": {"range7d": False, "values": wave(BUCKETS_24H, 20, 80)},
        "battery": 100,
        "now": NOW,
    },
    "tiny-values": {  # single digits everywhere
        "weather": weather(temp=8, hi=9, lo=2, code=45, wind=1, wind_dir=90),
        "aq": {"aqi": 3, "pm25": 3, "pm10": 1},
        "gov": 5,
        "history": {"range7d": False, "values": wave(BUCKETS_24H, 1, 6)},
        "battery": 3,
        "now": NOW,
    },
    "week": {  # 7-day range with data gaps
        "weather": weather(),
        "aq": {"aqi": 77, "pm25": 77, "pm10": 41},
        "gov": 65,
        "history": {"range7d": True,
                    "values": wave(BUCKETS_7D, 30, 180,
                                   gaps={20, 21, 22, 40})},
        "battery": 66,
        "now": NOW,
    },
    "no-history": {  # chart renders "no data"
        "weather": weather(),
        "aq": {"aqi": 42, "pm25": 42, "pm10": 18},
        "gov": 39,
        "history": {"range7d": False, "values": []},
        "battery": 71,
        "now": NOW,
    },
    "all-missing": {},  # every section renders dashes
}


def list_ports(hint):
    return [hint] if hint else sorted(glob.glob("/dev/ttyACM*"))


def open_testmode(port_hint) -> serial.Serial:
    """Get the device into a serial session.

    The port only exists while the device is awake. If present, hard-reset
    via the USB control lines to land in the boot listening window;
    otherwise ask for a manual RESET press and grab the port when it
    appears.
    """
    ports = list_ports(port_hint)
    if ports:
        with serial.Serial(ports[0]) as s:
            s.dtr = False
            s.rts = True
            time.sleep(0.1)
            s.rts = False
    else:
        print("no serial port — the device is deep-sleeping.")
        print("press the RESET button on the device now...")

    deadline = time.time() + 45
    while time.time() < deadline:
        ports = list_ports(port_hint)
        if ports:
            try:
                s = serial.Serial(ports[0], 115200, timeout=0.2)
                s.write(b"\n")  # knock: any byte enters test mode
                print(f"connected on {ports[0]}")
                return s
            except (OSError, serial.SerialException):
                pass
        time.sleep(0.2)
    sys.exit("no port appeared — is the device plugged in?")


def send(s: serial.Serial, payload: dict, label: str):
    # If the knock missed the boot window the device first finishes its
    # normal refresh cycle (fetch + e-ink update), so allow generous time.
    print(f"→ {label}")
    s.write((json.dumps(payload) + "\n").encode())
    deadline = time.time() + 90
    while time.time() < deadline:
        line = s.readline().decode(errors="replace").strip()
        if line:
            print(f"  {line}")
        if "rendered" in line or "bad json" in line:
            return
    print("  (no render ack — device may still be busy)")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("scenario", nargs="?", default="normal",
                    help="scenario name, 'all', or a raw JSON payload")
    ap.add_argument("--port", default=None)
    ap.add_argument("--list", action="store_true")
    args = ap.parse_args()

    if args.list:
        print("\n".join(SCENARIOS))
        return

    s = open_testmode(args.port)
    time.sleep(0.3)

    if args.scenario == "all":
        for name, payload in SCENARIOS.items():
            send(s, payload, name)
            input("  [enter] for next scenario...")
    elif args.scenario.lstrip().startswith("{"):
        send(s, json.loads(args.scenario), "custom payload")
    elif args.scenario in SCENARIOS:
        send(s, SCENARIOS[args.scenario], args.scenario)
    else:
        sys.exit(f"unknown scenario {args.scenario!r} — try --list")

    print("interactive: paste JSON, 'full', or 'exit' (^C to quit)")
    try:
        while True:
            line = input("> ").strip()
            if not line:
                continue
            s.write((line + "\n").encode())
            time.sleep(0.5)
            while s.in_waiting:
                print(" ", s.readline().decode(errors="replace").strip())
            if line == "exit":
                break
    except (KeyboardInterrupt, EOFError):
        pass
    s.close()


if __name__ == "__main__":
    main()
