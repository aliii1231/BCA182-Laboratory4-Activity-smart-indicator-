#!/usr/bin/env python3
"""Print average live readings for a few seconds, so you do not need the VS Code
monitor. Does not send anything to the board. Close other serial programs first.

  python tools/live_check.py --port COM5 --seconds 10
"""
import argparse
import re
import time

import serial

ap = argparse.ArgumentParser()
ap.add_argument("--port", required=True)
ap.add_argument("--baud", type=int, default=115200)
ap.add_argument("--seconds", type=float, default=10)
a = ap.parse_args()

SENSOR = re.compile(r"temp_c=([\d.\-]+),rh_pct=([\d.\-]+),ambient_raw=(\d+),proximity_raw=(\d+)")
vals = []
with serial.Serial(a.port, a.baud, timeout=1.0) as ser:
    end = time.time() + a.seconds
    while time.time() < end:
        line = ser.readline().decode("ascii", "replace").strip()
        m = SENSOR.search(line)
        if m:
            vals.append(tuple(float(x) for x in m.groups()))
        elif line.startswith("DATA,"):
            f = line.split(",")
            if len(f) == 11 and "NA" not in (f[5], f[6]):
                vals.append((float(f[5]), float(f[6]), float(f[7]), float(f[8])))
if not vals:
    print("No readings received. Is the board powered, reset, and the port free?")
else:
    n = len(vals)
    print(f"{n} readings: temp {sum(v[0] for v in vals)/n:.2f} C | "
          f"RH {sum(v[1] for v in vals)/n:.1f} % | "
          f"ambient max {max(v[2] for v in vals):.0f} | "
          f"proximity max {max(v[3] for v in vals):.0f}")