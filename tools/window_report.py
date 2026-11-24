#!/usr/bin/env python3
"""Per-window consistency report. Run from the repo root (needs qc_common.py).

For every 10-row window of each train_/test_ session it applies the PROVISIONAL
rule against the nearest-in-time State 0 baseline and marks windows whose rule
result differs from the label with "!".
"""
import qc_common as q

if __name__ == "__main__":
    data, times = q.load_all(), q.manifest_times()
    totals = {}
    for name, rows in data.items():
        v = q.valid(rows)
        if not v:
            continue
        label = int(v[0]["label"])
        base, src, gap = q.baseline_for(name, data, times)
        gtxt = f", {gap:.0f} min away" if gap else ""
        print(f"{name}  (baseline T={base[0]:.2f} RH={base[1]:.2f}; {src}{gtxt})")
        good = q.trusted(src, gap)
        if not good:
            print("  (baseline is stale: humidity/temperature not judged)")
        for i, (t, h, a, p) in enumerate(q.windows(v), 1):
            risk = good and ((h - base[1]) >= q.RH_D or (t - base[0]) >= q.T_D)
            pred = 2 if risk else (1 if (a >= q.AMB or p >= q.PROX) else 0)
            if not good and label == 2:
                print(f"  w{i} T {t:5.2f} RH {h:5.1f} (d {h - base[1]:+5.1f}) "
                      f"amb_max {a:4d} prox_max {p:4d} rule=? (needs fresh State 0)")
                continue
            bad = pred != label
            tot = totals.setdefault(label, [0, 0])
            tot[0] += 1
            tot[1] += 0 if bad else 1
            print(f"  w{i} T {t:5.2f} RH {h:5.1f} (d {h - base[1]:+5.1f}) "
                  f"amb_max {a:4d} prox_max {p:4d} rule={pred} {'!' if bad else ' '}")
    print("\nwindows where the rule agrees with the label:")
    for lab in sorted(totals):
        n, ok = totals[lab]
        print(f"  State {lab}: {ok}/{n}")
