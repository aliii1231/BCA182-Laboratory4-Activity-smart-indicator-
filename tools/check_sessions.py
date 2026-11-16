#!/usr/bin/env python3
"""Quick QC of recorded train_/test_ sessions in data/raw. Run from the repo root.

Prints one line per session and flags anything that looks wrong. The baseline
values below are PROVISIONAL (measured in the closed box with outputs idle);
edit them if your measured idle baseline changes.
"""
import csv
import glob
import os

BASE_T = 32.3      # deg C, closed box, outputs idle
BASE_RH = 77.0     # percent RH, closed box, outputs idle
MIN_ROWS = 56      # a 62 s recording minus 2 warm-up rows is about 60

for path in sorted(glob.glob("data/raw/*.csv")):
    name = os.path.basename(path)[:-4]
    if not (name.startswith("train_") or name.startswith("test_")):
        continue
    rows = list(csv.DictReader(open(path)))
    if not rows:
        print(f"{name:14s} NO ROWS")
        continue
    ok = [r for r in rows if r["temp_c"] != "NA" and r["rh_pct"] != "NA"]
    labels = {r["label"] for r in rows}
    pressed = sum(1 for r in rows if r["buttons"] != "0")
    t = [float(r["temp_c"]) for r in ok]
    h = [float(r["rh_pct"]) for r in ok]
    a = [int(r["ambient_raw"]) for r in rows]
    p = [int(r["proximity_raw"]) for r in rows]
    state = next(iter(labels)) if len(labels) == 1 else "?"
    flags = []
    if len(rows) < MIN_ROWS:
        flags.append("SHORT")
    if len(ok) != len(rows):
        flags.append("NA_ROWS")
    if len(labels) != 1:
        flags.append("MIXED_LABELS")
    if pressed:
        flags.append("BUTTON_PRESSED")
    if state == "0" and (max(a) >= 20 or max(p) >= 100):
        flags.append("LIGHT_OR_HAND_IN_STATE0")
    if state == "1" and max(a) < 20 and max(p) < 100:
        flags.append("NO_ACCESS_SIGNAL")
    if state == "2" and (max(h) - BASE_RH) < 4 and (max(t) - BASE_T) < 1.5:
        flags.append("NO_RISK_SIGNAL")
    print(f"{name:14s} L{state} rows={len(rows):3d} "
          f"T {min(t):.1f}-{max(t):.1f} RH {min(h):.1f}-{max(h):.1f} "
          f"amb max {max(a):4d} prox {min(p):4d}-{max(p):4d} "
          f"{' '.join(flags) if flags else 'OK'}")
