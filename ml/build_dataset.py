#!/usr/bin/env python3
"""Build window-level train/test datasets using the QC tools' window rules."""
import glob
import json
import os
import sys
import tempfile

import numpy as np
import pandas as pd

sys.path.insert(0, "tools")
import qc_common as q

FEATURES = [
    "amb_mean", "amb_max", "amb_std", "prox_mean", "prox_max", "prox_min",
    "prox_std", "temp_mean", "rh_mean", "temp_delta", "rh_delta",
]
RELATIVE_FEATURES = ["temp_rel", "rh_rel"]
MAX_GAP_MIN = 180
DESCRIPTIONS = {
    "amb_mean": "Mean ambient light reading across the window.",
    "amb_max": "Maximum ambient light reading across the window.",
    "amb_std": "Population standard deviation of ambient light readings across the window.",
    "prox_mean": "Mean proximity reading across the window.",
    "prox_max": "Maximum proximity reading across the window.",
    "prox_min": "Minimum proximity reading across the window.",
    "prox_std": "Population standard deviation of proximity readings across the window.",
    "temp_mean": "Mean temperature in degrees Celsius across the window.",
    "rh_mean": "Mean relative humidity percentage across the window.",
    "temp_delta": "Last minus first temperature in degrees Celsius within the window.",
    "rh_delta": "Last minus first relative humidity percentage within the window.",
}
OUTPUT_COLUMNS = ["session_id", "split", "window", "label", *FEATURES,
                  *RELATIVE_FEATURES]


def features(rows):
    amb = np.asarray([float(r["ambient_raw"]) for r in rows], dtype=np.float64)
    prox = np.asarray([float(r["proximity_raw"]) for r in rows], dtype=np.float64)
    temp = np.asarray([float(r["temp_c"]) for r in rows], dtype=np.float64)
    rh = np.asarray([float(r["rh_pct"]) for r in rows], dtype=np.float64)
    return {
        "amb_mean": np.mean(amb, dtype=np.float64), "amb_max": np.max(amb),
        "amb_std": np.std(amb, ddof=0, dtype=np.float64),
        "prox_mean": np.mean(prox, dtype=np.float64), "prox_max": np.max(prox),
        "prox_min": np.min(prox), "prox_std": np.std(prox, ddof=0, dtype=np.float64),
        "temp_mean": np.mean(temp, dtype=np.float64), "rh_mean": np.mean(rh, dtype=np.float64),
        "temp_delta": temp[-1] - temp[0], "rh_delta": rh[-1] - rh[0],
    }


def main():
    exclusions_path = os.path.join("data", "exclusions.csv")
    if not os.path.isfile(exclusions_path):
        raise SystemExit(f"Error: required exclusions file is missing: {exclusions_path}")
    exclusions = q.load_exclusions(exclusions_path)
    if not exclusions:
        # DictReader treats a leading blank line as the CSV header. Normalize a
        # temporary copy so the QC helper can still parse the supplied file.
        with open(exclusions_path, encoding="utf-8-sig") as source:
            contents = source.read().lstrip("\r\n")
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", newline="",
                                         suffix=".csv", delete=False) as normalized:
            normalized.write(contents)
            normalized_path = normalized.name
        try:
            exclusions = q.load_exclusions(normalized_path)
        finally:
            os.unlink(normalized_path)
    times = q.manifest_times()
    records = {"train": [], "test": []}
    counts = {"train": {}, "test": {}}
    relative_counts = {"train": {}, "test": {}}
    session_lines = []
    excluded_lines = []
    reported_exclusions = set()
    source_sessions = {}
    baselines = {"train": [], "test": []}
    for split in ("train", "test"):
        paths = sorted(glob.glob(os.path.join("data", "raw", f"{split}_*.csv")))
        for path in paths:
            rows = q.load(path)
            if not rows:
                continue
            name = os.path.basename(path)[:-4]
            source_sessions[name] = rows
            if rows[0]["label"] == "0" and name in times:
                valid = q.valid(rows)
                if valid:
                    baselines[split].append((name, times[name],
                                             float(np.mean([float(r["temp_c"]) for r in valid], dtype=np.float64)),
                                             float(np.mean([float(r["rh_pct"]) for r in valid], dtype=np.float64))))
    for split in ("train", "test"):
        for path in sorted(glob.glob(os.path.join("data", "raw", f"{split}_*.csv"))):
            name = os.path.basename(path)[:-4]
            rows = source_sessions.get(name, [])
            if not rows:
                continue
            excluded = exclusions.get(name, {"all": None, "wins": {}})
            valid = q.valid(rows)
            total_windows = len(valid) // q.WIN
            if excluded["all"]:
                session_lines.append(f"{name}: windows built=0, windows excluded={total_windows}")
                excluded_lines.append(f"{name} all: {excluded['all']}")
                reported_exclusions.add((name, "all"))
                continue
            excluded_numbers = sorted(n for n in excluded["wins"] if 1 <= n <= total_windows)
            for number in excluded_numbers:
                excluded_lines.append(f"{name} w{number}: {excluded['wins'][number]}")
                reported_exclusions.add((name, f"w{number}"))
            label = int(rows[0]["label"])
            built = 0
            for number in range(1, total_windows + 1):
                if number in excluded["wins"]:
                    continue
                window_rows = valid[(number - 1) * q.WIN:number * q.WIN]
                candidates = [(abs((when - times[name]).total_seconds()), base_temp, base_rh)
                              for base_name, when, base_temp, base_rh in baselines[split]
                              if base_name != name and name in times]
                nearest = min(candidates, key=lambda item: item[0]) if candidates else None
                if nearest is not None and nearest[0] <= MAX_GAP_MIN * 60:
                    temp_rel = float(np.mean([float(r["temp_c"]) for r in window_rows], dtype=np.float64)) - nearest[1]
                    rh_rel = float(np.mean([float(r["rh_pct"]) for r in window_rows], dtype=np.float64)) - nearest[2]
                else:
                    temp_rel = rh_rel = float("nan")
                record = {"session_id": name, "split": split, "window": number,
                          "label": label, **features(window_rows),
                          "temp_rel": temp_rel, "rh_rel": rh_rel}
                records[split].append(record)
                counts[split][label] = counts[split].get(label, 0) + 1
                if not np.isnan(temp_rel) and not np.isnan(rh_rel):
                    relative_counts[split][label] = relative_counts[split].get(label, 0) + 1
                built += 1
            session_lines.append(
                f"{name}: windows built={built}, windows excluded={len(excluded_numbers)}")

    for name, excluded in sorted(exclusions.items()):
        if excluded["all"] and (name, "all") not in reported_exclusions:
            excluded_lines.append(f"{name} all: {excluded['all']}")
        for number, reason in sorted(excluded["wins"].items()):
            if (name, f"w{number}") not in reported_exclusions:
                excluded_lines.append(f"{name} w{number}: {reason}")

    os.makedirs("ml/out", exist_ok=True)
    for split in ("train", "test"):
        pd.DataFrame(records[split], columns=OUTPUT_COLUMNS).to_csv(
            f"ml/out/{split}_windows.csv", index=False)
    with open("ml/out/features.json", "w", encoding="utf-8") as f:
        json.dump({"features": [*FEATURES, *RELATIVE_FEATURES], "window_length": q.WIN,
                   "sampling_hz": 1, "definitions": DESCRIPTIONS,
                   "relative_features": {
                       "features": RELATIVE_FEATURES,
                       "baseline": "Mean temp_c and rh_pct of the nearest-in-time other State 0 session in the same split, using data/manifest.csv start times.",
                       "max_gap_minutes": MAX_GAP_MIN,
                       "missing_baseline": "NaN when no other State 0 session has a manifest time within the maximum gap."
                   }}, f, indent=2)
        f.write("\n")

    report = ["# Dataset build report", "", "## Per session", "", *session_lines,
              "", "## Excluded sessions and windows", ""]
    report.extend(excluded_lines if excluded_lines else ["None."])
    report.extend(["", "## Windows per split and state", "", "| Split | State | Windows |",
                   "|---|---:|---:|"])
    errors = []
    for split in ("train", "test"):
        minimum = 30 if split == "train" else 10
        for state in (0, 1, 2):
            count = counts[split].get(state, 0)
            report.append(f"| {split} | {state} | {count} |")
            if count < minimum:
                errors.append(f"{split} State {state} has {count} windows; requires {minimum}")
    report.extend(["", "## Windows with relative baseline", "",
                   "| Split | State | Windows with baseline |", "|---|---:|---:|"])
    for split in ("train", "test"):
        for state in (0, 1, 2):
            report.append(f"| {split} | {state} | {relative_counts[split].get(state, 0)} |")
    report_text = "\n".join(report) + "\n"
    print(report_text, end="")
    with open("ml/out/dataset_report.md", "w", encoding="utf-8") as f:
        f.write(report_text)
    if errors:
        raise SystemExit("\n".join(errors))


if __name__ == "__main__":
    main()
