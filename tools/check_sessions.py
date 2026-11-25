#!/usr/bin/env python3
"""QC of recorded train_/test_ sessions in data/raw. Run from the repo root.

Each session is compared with the State 0 session closest in time (from
data/manifest.csv), because the room drifts by several %RH and 1-2 C within an
hour. Windows are 10 rows. A session is flagged WEAK when fewer than MIN_OK of
its windows support its label. Record a State 0 session right before each
State 1 / State 2 session so the baseline is fresh.
Thresholds are PROVISIONAL: RH +4 %, T +1.5 C, ambient_raw 20, proximity_raw 100.
"""
import qc_common as q

MIN_ROWS = 56

if __name__ == "__main__":
    data, times = q.load_all(), q.manifest_times()
    missing = [n for n in data if n not in times]
    print(f"manifest start times found for {len(data) - len(missing)}/{len(data)} sessions"
          + (f" (missing: {', '.join(missing)})" if missing else ""))
    for name, rows in data.items():
        if not rows:
            print(f"{name:12s} NO ROWS")
            continue
        v = q.valid(rows)
        labels = {r["label"] for r in rows}
        label = int(next(iter(labels))) if len(labels) == 1 else -1
        base, src, gap = q.baseline_for(name, data, times)
        ws = list(q.windows(v))
        good = q.trusted(src, gap)
        res = [q.supports(label, w, base, good) for w in ws]
        ok = sum(1 for r in res if r)
        flags = []
        if len(rows) < MIN_ROWS: flags.append("SHORT")
        if len(v) != len(rows): flags.append("NA_ROWS")
        if len(labels) != 1: flags.append("MIXED_LABELS")
        if any(r["buttons"] != "0" for r in rows): flags.append("BUTTON_PRESSED")
        if not good: flags.append("BASELINE_OLD(humidity/temp unverified)")
        if label == 2 and not good:
            flags.append("NEEDS_BASELINE")
        elif ok < -(-2 * len(ws) // 3):      # fewer than 2/3 of the windows
            flags.append("WEAK")
        gtxt = f", {gap:.0f} min away" if gap else ""
        print(f"{name:12s} L{label} rows={len(rows):3d} base T{base[0]:.1f}/RH{base[1]:.1f} "
              f"({src}{gtxt}) windows_ok={ok if not (label == 2 and not good) else '?'}/{len(ws)} {' '.join(flags) if flags else 'OK'}")
