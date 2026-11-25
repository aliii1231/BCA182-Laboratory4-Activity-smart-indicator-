#!/usr/bin/env python3
"""Build window-level train/test datasets using the QC tools' window rules."""
import glob
import json
import os
import sys

import numpy as np
import pandas as pd

sys.path.insert(0, "tools")
import qc_common as q

FEATURES = [
    "amb_mean", "amb_max", "amb_std", "prox_mean", "prox_max", "prox_min",
    "prox_std", "temp_mean", "rh_mean", "temp_delta", "rh_delta",
]
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
OUTPUT_COLUMNS = ["session_id", "split", "window", "label", *FEATURES]


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
    exclusions = q.load_exclusions()
    records = {"train": [], "test": []}
    counts = {"train": {}, "test": {}}
    for split in ("train", "test"):
        for path in sorted(glob.glob(os.path.join("data", "raw", f"{split}_*.csv"))):
            rows = q.load(path)
            if not rows:
                continue
            name = os.path.basename(path)[:-4]
            excluded = exclusions.get(name, {"all": None, "wins": {}})
            if excluded["all"]:
                continue
            valid = q.valid(rows)
            label = int(rows[0]["label"])
            for number in range(1, len(valid) // q.WIN + 1):
                if number in excluded["wins"]:
                    continue
                window_rows = valid[(number - 1) * q.WIN:number * q.WIN]
                record = {"session_id": name, "split": split, "window": number,
                          "label": label, **features(window_rows)}
                records[split].append(record)
                counts[split][label] = counts[split].get(label, 0) + 1

    os.makedirs("ml/out", exist_ok=True)
    for split in ("train", "test"):
        pd.DataFrame(records[split], columns=OUTPUT_COLUMNS).to_csv(
            f"ml/out/{split}_windows.csv", index=False)
    with open("ml/out/features.json", "w", encoding="utf-8") as f:
        json.dump({"features": FEATURES, "window_length": q.WIN, "sampling_hz": 1,
                   "definitions": DESCRIPTIONS}, f, indent=2)
        f.write("\n")

    print("windows per split and state:")
    errors = []
    for split in ("train", "test"):
        minimum = 30 if split == "train" else 10
        for state in sorted(counts[split]):
            count = counts[split][state]
            print(f"  {split} State {state}: {count}")
            if count < minimum:
                errors.append(f"{split} State {state} has {count} windows; requires {minimum}")
    if errors:
        raise SystemExit("\n".join(errors))


if __name__ == "__main__":
    main()
