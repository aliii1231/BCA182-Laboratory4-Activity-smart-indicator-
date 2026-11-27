#!/usr/bin/env python3
"""Summarize data/trials/trials.csv: per-state success rate, confusion matrix, delay."""
import csv
import statistics

rows = list(csv.DictReader(open("data/trials/trials.csv", newline="", encoding="utf-8-sig")))
names = {0: "SAFE", 1: "OPEN", 2: "RISK", 255: "WAIT"}
print(f"{len(rows)} trials\n")
conf = {}
for r in rows:
    key = (int(r["expected"]), int(r["final_shown"]))
    conf[key] = conf.get(key, 0) + 1
cols = [0, 1, 2, 255]
print("confusion (rows = expected, columns = final shown state)")
print("expected \\ shown  " + "  ".join(f"{names[c]:>5}" for c in cols))
for s in (0, 1, 2):
    print(f"{names[s]:>16}  " + "  ".join(f"{conf.get((s, c), 0):>5}" for c in cols))
print()
for s in (0, 1, 2):
    rs = [r for r in rows if int(r["expected"]) == s]
    if not rs:
        continue
    ok = sum(1 for r in rs if r["verdict"] == "correct")
    delays = [float(r["first_correct_s"]) for r in rs if r["first_correct_s"]]
    d = f", median first-correct {statistics.median(delays):.1f} s" if delays else ""
    print(f"State {s} ({names[s]}): {ok}/{len(rs)} correct ({100 * ok / len(rs):.0f} %){d}")
