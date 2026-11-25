"""Shared helpers for check_sessions.py and window_report.py (keep in tools/)."""
import csv
import datetime as dt
import glob
import os
import re

WIN, RH_D, T_D, AMB, PROX = 10, 4.0, 1.5, 20, 100     # PROVISIONAL thresholds
FALLBACK = (32.3, 77.0)
OLD_GAP_MIN = 35


def mean(v):
    return sum(v) / len(v)


def load(path):
    return list(csv.DictReader(open(path, newline="", encoding="utf-8-sig")))


def valid(rows):
    return [r for r in rows if r["temp_c"] != "NA" and r["rh_pct"] != "NA"]


def windows(rows):
    for i in range(len(rows) // WIN):
        c = rows[i * WIN:(i + 1) * WIN]
        yield (mean([float(r["temp_c"]) for r in c]), mean([float(r["rh_pct"]) for r in c]),
               max(int(r["ambient_raw"]) for r in c), max(int(r["proximity_raw"]) for r in c))


def load_all(raw_dir="data/raw"):
    return {os.path.basename(p)[:-4]: load(p)
            for p in sorted(glob.glob(os.path.join(raw_dir, "*.csv")))
            if os.path.basename(p).startswith(("train_", "test_"))}


def manifest_times(path="data/manifest.csv"):
    """session_id -> start time. If a session appears twice, the later row wins."""
    times = {}
    if os.path.exists(path):
        for r in csv.DictReader(open(path, newline="", encoding="utf-8-sig")):
            sid = (r.get("session_id") or "").strip()
            s = (r.get("start_utc") or "").strip()
            if sid and s:
                try:
                    times[sid] = dt.datetime.fromisoformat(s)
                except ValueError:
                    pass
    return times


def _mean_tr(rows):
    v = valid(rows)
    if not v:
        return None
    return (mean([float(r["temp_c"]) for r in v]), mean([float(r["rh_pct"]) for r in v]))


def baseline_for(name, data, times):
    """Return ((T, RH), description, gap_minutes_or_None).

    Uses the State 0 session of the same split (train/test) that is closest in
    time according to the manifest. Falls back to the same pass number, then to
    fixed constants. A State 0 session is its own baseline."""
    sp = name.split("_")[0]
    s0 = [n for n, rows in data.items()
          if n.split("_")[0] == sp and rows and rows[0]["label"] == "0"]
    if name in s0:
        b = _mean_tr(data[name])
        if b:
            return b, "itself", 0.0
    if name in times:
        timed = [n for n in s0 if n in times]
        if timed:
            ref = min(timed, key=lambda n: abs((times[n] - times[name]).total_seconds()))
            b = _mean_tr(data[ref])
            if b:
                gap = abs((times[ref] - times[name]).total_seconds()) / 60
                return b, f"nearest S0 {ref}", gap
    m = re.match(r"(train|test)_s\d_(\w+)$", name)
    if m:
        ref = f"{m.group(1)}_s0_{m.group(2)}"
        if ref in data:
            b = _mean_tr(data[ref])
            if b:
                return b, f"same-pass {ref}", None
    return FALLBACK, "FALLBACK", None


def trusted(src, gap):
    """A baseline is trusted if it is the session itself or a State 0 session
    recorded within OLD_GAP_MIN minutes of it."""
    return src == "itself" or (gap is not None and gap <= OLD_GAP_MIN)


def supports(label, w, base, ok_baseline=True):
    """True/False whether the window supports its label, or None when the label
    cannot be judged because the baseline is stale (State 2 only)."""
    t, h, a, p = w
    risk = (h - base[1]) >= RH_D or (t - base[0]) >= T_D
    access = a >= AMB or p >= PROX
    if label == 2:
        return risk if ok_baseline else None
    if not ok_baseline:                 # stale baseline: humidity/temp unverified
        return access if label == 1 else not access
    if label == 1:
        return access and not risk
    return not risk and not access
