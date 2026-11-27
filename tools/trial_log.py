#!/usr/bin/env python3
"""Record one product trial and append the outcome to data/trials/trials.csv.

  python tools/trial_log.py --port COM5 --state 1 --trial 3 --seconds 60 --notes "flashlight 10 cm"

The trial is CORRECT when the product's stable state ("shown") equals the
expected state for at least 80 % of the PRED lines from 20 s on (after the
window has filled) AND the last shown state is the expected one. Trials are
never used to train or tune anything. Do not repeat a trial because the result
was wrong; repeat it only for a recording fault, and say so in the notes.
"""
import argparse
import csv
import datetime as dt
import os
import re
import time

PRED = re.compile(r"PRED,tick_ms=(\d+),fill=(\d+),pred=(\d+),shown=(\d+)")
FIELDS = ["trial_id", "expected", "start_utc", "seconds", "n_pred", "correct_frac",
          "final_shown", "first_correct_s", "verdict", "notes"]


def evaluate(samples, expected, seconds):
    """samples: list of (elapsed_s, fill, shown)."""
    ready = [(t, s) for t, f, s in samples if f >= 10]
    late = [s for t, s in ready if t >= 20]
    frac = (sum(1 for s in late if s == expected) / len(late)) if late else 0.0
    first = next((t for t, s in ready if s == expected), None)
    final = samples[-1][2] if samples else 255
    verdict = "correct" if (late and frac >= 0.8 and final == expected) else "wrong"
    return frac, final, first, verdict


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--state", type=int, choices=[0, 1, 2], required=True)
    ap.add_argument("--trial", type=int, required=True)
    ap.add_argument("--seconds", type=float, default=60)
    ap.add_argument("--notes", default="")
    ap.add_argument("--replay")
    ap.add_argument("--out", default="data/trials")
    ap.add_argument("--force", action="store_true")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    tid = f"s{a.state}_{a.trial:02d}"
    csv_path = os.path.join(a.out, "trials.csv")
    if os.path.exists(csv_path) and not a.force:
        for r in csv.DictReader(open(csv_path, newline="", encoding="utf-8-sig")):
            if r["trial_id"] == tid:
                raise SystemExit(f"Trial {tid} already exists. Use a new trial number.")
    raw = []
    samples = []
    start_utc = dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds")
    if a.replay:
        n = 0
        for ln in open(a.replay, encoding="utf-8", errors="replace").read().splitlines():
            m = PRED.search(ln)
            if m:
                samples.append((float(n), int(m.group(2)), int(m.group(4))))
                n += 1
            raw.append(ln)
    else:
        if not a.port:
            ap.error("--port is required unless --replay is used")
        import serial
        with serial.Serial(a.port, a.baud, timeout=1.0) as ser:
            ser.reset_input_buffer()
            t0 = time.time()
            print(f"Trial {tid}: recording {a.seconds:g} s ...")
            while time.time() - t0 < a.seconds:
                ln = ser.readline().decode("ascii", "replace").strip()
                if not ln:
                    continue
                raw.append(ln)
                m = PRED.search(ln)
                if m:
                    samples.append((time.time() - t0, int(m.group(2)), int(m.group(4))))
    open(os.path.join(a.out, f"trial_{tid}.log"), "w", encoding="utf-8").write("\n".join(raw) + "\n")
    frac, final, first, verdict = evaluate(samples, a.state, a.seconds)
    row = [tid, a.state, start_utc, a.seconds, len(samples), f"{frac:.2f}", final,
           "" if first is None else f"{first:.1f}", verdict, a.notes]
    new = not os.path.exists(csv_path)
    with open(csv_path, "a", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        if new:
            w.writerow(FIELDS)
        w.writerow(row)
    print(f"{tid}: {verdict}  (shown correct {frac * 100:.0f} % of the time after 20 s, "
          f"final shown={final}, first correct at {row[7] or 'never'} s)")


if __name__ == "__main__":
    main()
