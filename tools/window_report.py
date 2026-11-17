#!/usr/bin/env python3
"""Per-window consistency report for recorded sessions. Run from the repo root.

Splits each train_/test_ session into non-overlapping 10-row windows and applies
the PROVISIONAL baseline rule:
  State 2 if RH >= baseline+RH_DELTA or temp >= baseline+T_DELTA (window mean)
  State 1 if ambient_raw max >= AMB_OPEN or proximity_raw max >= PROX_NEAR
  State 0 otherwise
The baseline comes from train_ State 0 sessions only (test data never sets it).
A window whose rule result differs from its label is marked "!" - these are the
windows to look at (label noise, or a setup that did not produce the state).
"""
import csv
import glob
import os
import sys

WIN = 10
RH_DELTA = 4.0
T_DELTA = 1.5
AMB_OPEN = 20
PROX_NEAR = 100


def mean(v):
    return sum(v) / len(v)


def median(v):
    v = sorted(v)
    n = len(v)
    return v[n // 2] if n % 2 else (v[n // 2 - 1] + v[n // 2]) / 2


def load(path):
    return list(csv.DictReader(open(path)))


paths = [p for p in sorted(glob.glob("data/raw/*.csv"))
         if os.path.basename(p).startswith(("train_", "test_"))]
if not paths:
    sys.exit("no train_/test_ sessions found in data/raw")

t0, h0 = [], []
for p in paths:
    if not os.path.basename(p).startswith("train_"):
        continue
    rs = load(p)
    if rs and {r["label"] for r in rs} == {"0"}:
        t0 += [float(r["temp_c"]) for r in rs if r["temp_c"] != "NA"]
        h0 += [float(r["rh_pct"]) for r in rs if r["rh_pct"] != "NA"]
if not t0:
    sys.exit("need at least one train_ State 0 session for the baseline")
BASE_T, BASE_RH = median(t0), median(h0)
print(f"baseline (train State 0 only): T={BASE_T:.2f} C  RH={BASE_RH:.2f} %\n")

totals = {}
for p in paths:
    name = os.path.basename(p)[:-4]
    rows = [r for r in load(p) if r["temp_c"] != "NA" and r["rh_pct"] != "NA"]
    if not rows:
        continue
    label = int(rows[0]["label"])
    print(name)
    for w in range(len(rows) // WIN):
        ch = rows[w * WIN:(w + 1) * WIN]
        t = mean([float(r["temp_c"]) for r in ch])
        h = mean([float(r["rh_pct"]) for r in ch])
        amax = max(int(r["ambient_raw"]) for r in ch)
        pmax = max(int(r["proximity_raw"]) for r in ch)
        if (h - BASE_RH) >= RH_DELTA or (t - BASE_T) >= T_DELTA:
            pred = 2
        elif amax >= AMB_OPEN or pmax >= PROX_NEAR:
            pred = 1
        else:
            pred = 0
        bad = pred != label
        a = totals.setdefault(label, [0, 0])
        a[0] += 1
        a[1] += 0 if bad else 1
        print(f"  w{w + 1} T {t:5.2f} RH {h:5.1f} (d {h - BASE_RH:+5.1f}) "
              f"amb_max {amax:4d} prox_max {pmax:4d} rule={pred} "
              f"{'!' if bad else ' '}")
print("\nwindows consistent with label (rule vs label):")
for lab in sorted(totals):
    n, ok = totals[lab]
    print(f"  State {lab}: {ok}/{n}")