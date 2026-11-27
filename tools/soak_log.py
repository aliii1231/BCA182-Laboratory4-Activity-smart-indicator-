#!/usr/bin/env python3
"""Record the product's serial output for a fixed time and summarize reliability.

  python tools/soak_log.py --port COM5 --minutes 15 --name soak1
  python tools/soak_log.py --replay some.log --name test      (no board needed)

Writes data/trials/<name>.log (raw lines) and data/trials/<name>_summary.txt.
Close other serial programs first. The board must be in product mode.
"""
import argparse
import os
import re
import time

PRED = re.compile(r"PRED,tick_ms=(\d+),fill=(\d+),pred=(\d+),shown=(\d+).*?fault=(\d+)")
PROD = re.compile(r"PRODUCT,tick_ms=(\d+),shown=(\d+),fault=(\d+),drops=(\d+),"
                  r"stack_hw_sensor=(\d+),stack_hw_infer=(\d+),stack_hw_ui=(\d+),"
                  r"stack_hw_console=(\d+),heap_free=(\d+),heap_min=(\d+)")


def summarize(lines):
    ticks, shown, faults, preds = [], {}, 0, 0
    prods = []
    for ln in lines:
        m = PRED.search(ln)
        if m:
            preds += 1
            ticks.append(int(m.group(1)))
            shown[m.group(4)] = shown.get(m.group(4), 0) + 1
            faults += int(m.group(5))
        m = PROD.search(ln)
        if m:
            prods.append([int(x) for x in m.groups()])
    out = []
    if not ticks:
        return ["No PRED lines found. Is the board in product mode and the port free?"]
    gaps = [b - a for a, b in zip(ticks, ticks[1:])]
    resets = sum(1 for g in gaps if g < 0)
    pos = [g for g in gaps if g >= 0]
    dur = (ticks[-1] - ticks[0]) / 1000.0
    out.append(f"duration (board ticks)  : {dur:.0f} s")
    out.append(f"PRED lines              : {preds} (expected about {dur:.0f})")
    out.append(f"max gap between PRED    : {max(pos) if pos else 0} ms")
    out.append(f"gaps over 1500 ms       : {sum(1 for g in pos if g > 1500)}")
    out.append(f"possible hangs (>5 s)   : {sum(1 for g in pos if g > 5000)}")
    out.append(f"board restarts (tick drop): {resets}")
    out.append(f"PRED lines with fault=1 : {faults}")
    out.append("shown-state histogram   : " + ", ".join(f"{k}:{v}" for k, v in sorted(shown.items())))
    if prods:
        out.append(f"PRODUCT lines           : {len(prods)}")
        out.append(f"max dropped samples     : {max(p[3] for p in prods)}")
        out.append("min stack high-water    : sensor={} infer={} ui={} console={} words".format(
            *(min(p[i] for p in prods) for i in (4, 5, 6, 7))))
        out.append(f"min free heap ever      : {min(p[9] for p in prods)} bytes")
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--minutes", type=float, default=15)
    ap.add_argument("--name", required=True)
    ap.add_argument("--replay")
    a = ap.parse_args()
    os.makedirs("data/trials", exist_ok=True)
    raw_path = f"data/trials/{a.name}.log"
    lines = []
    if a.replay:
        lines = open(a.replay, encoding="utf-8", errors="replace").read().splitlines()
        open(raw_path, "w", encoding="utf-8").write("\n".join(lines) + "\n")
    else:
        if not a.port:
            ap.error("--port is required unless --replay is used")
        import serial
        end = time.time() + a.minutes * 60
        with serial.Serial(a.port, a.baud, timeout=1.0) as ser, \
                open(raw_path, "w", encoding="utf-8") as f:
            print(f"Recording {a.minutes:g} min to {raw_path} (Ctrl+C stops early) ...")
            try:
                while time.time() < end:
                    ln = ser.readline().decode("ascii", "replace").strip()
                    if ln:
                        f.write(ln + "\n")
                        lines.append(ln)
            except KeyboardInterrupt:
                print("stopped early")
    text = summarize(lines)
    open(f"data/trials/{a.name}_summary.txt", "w", encoding="utf-8").write("\n".join(text) + "\n")
    print("\n".join(text))


if __name__ == "__main__":
    main()