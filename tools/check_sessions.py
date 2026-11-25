#!/usr/bin/env python3
"""QC of recorded train_/test_ sessions in data/raw. Run from the repo root.

Each session is compared with the State 0 session closest in time (from
data/manifest.csv). Windows are 10 rows. A session is WEAK when fewer than 2/3
of its (non-excluded) windows support its label. Sessions and windows listed in
data/exclusions.csv are shown as EXCLUDED and left out of the counts.
Thresholds are PROVISIONAL: RH +4 %, T +1.5 C, ambient_raw 20, proximity_raw 100.
"""
import qc_common as q

MIN_ROWS = 56

if __name__ == "__main__":
    data, times, excl = q.load_all(), q.manifest_times(), q.load_exclusions()
    missing = [n for n in data if n not in times]
    print(f"manifest start times found for {len(data) - len(missing)}/{len(data)} sessions"
          + (f" (missing: {', '.join(missing)})" if missing else ""))
    usable = {}
    for name, rows in data.items():
        if not rows:
            print(f"{name:12s} NO ROWS")
            continue
        v = q.valid(rows)
        labels = {r["label"] for r in rows}
        label = int(next(iter(labels))) if len(labels) == 1 else -1
        e = excl.get(name, {"all": None, "wins": {}})
        if e["all"]:
            print(f"{name:12s} L{label} EXCLUDED: {e['all']}")
            continue
        base, src, gap = q.baseline_for(name, data, times)
        ws = [w for i, w in enumerate(q.windows(v), 1) if i not in e["wins"]]
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
        elif ws and ok < -(-2 * len(ws) // 3):
            flags.append("WEAK")
        gtxt = f", {gap:.0f} min away" if gap else ""
        xtxt = f" ({len(e['wins'])} excluded)" if e["wins"] else ""
        shown = ok if not (label == 2 and not good) else "?"
        print(f"{name:12s} L{label} rows={len(rows):3d} base T{base[0]:.1f}/RH{base[1]:.1f} "
              f"({src}{gtxt}) windows_ok={shown}/{len(ws)}{xtxt} {' '.join(flags) if flags else 'OK'}")
        key = (name.split("_")[0], label)
        usable[key] = usable.get(key, 0) + len(ws)
    print("\nusable windows after exclusions (train needs >= 30 per state, test >= 10):")
    for (split, label), n in sorted(usable.items()):
        need = 30 if split == "train" else 10
        print(f"  {split} State {label}: {n:3d}  {'OK' if n >= need else f'need {need - n} more'}")
