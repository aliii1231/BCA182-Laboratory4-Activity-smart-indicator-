#!/usr/bin/env python3
"""Add manifest rows for train_/test_ sessions that have no start time.

Run from the repo root, on the PC where the sessions were recorded (a git
clone resets file times). The start time is reconstructed as the file's last
write time minus the session length (62 s), and the note says so. Existing
rows are never edited; the tools use the LAST row of a session.

  python tools/rebuild_manifest.py --dry-run     (show what would be added)
  python tools/rebuild_manifest.py               (append the rows)
"""
import argparse
import csv
import datetime as dt
import glob
import os

import qc_common as q

ap = argparse.ArgumentParser()
ap.add_argument("--dry-run", action="store_true")
ap.add_argument("--seconds", type=float, default=62.0)
args = ap.parse_args()

MANIFEST = "data/manifest.csv"
HEADER = ["session_id", "label", "start_utc", "seconds_requested", "rows_kept",
          "rows_with_NA", "rows_skipped_warmup", "bad_lines", "notes"]
times = q.manifest_times(MANIFEST)
new_rows = []
for path in sorted(glob.glob("data/raw/*.csv")):
    name = os.path.basename(path)[:-4]
    if not name.startswith(("train_", "test_")) or name in times:
        continue
    rows = q.load(path)
    if not rows:
        continue
    end = dt.datetime.fromtimestamp(os.path.getmtime(path), dt.timezone.utc)
    start = (end - dt.timedelta(seconds=args.seconds)).replace(microsecond=0)
    na = sum(1 for r in rows if r["temp_c"] == "NA" or r["rh_pct"] == "NA")
    new_rows.append([name, rows[0]["label"], start.isoformat(), args.seconds,
                     len(rows), na, "", "",
                     "start_utc reconstructed from file modification time"])

if not new_rows:
    print("Every train_/test_ session already has a start time. Nothing to do.")
else:
    for r in new_rows:
        print("add:", r[0], "label", r[1], "start", r[2])
    if args.dry_run:
        print("(dry run, nothing written)")
    else:
        exists = os.path.exists(MANIFEST)
        if exists:
            raw = open(MANIFEST, "rb").read()
            if raw and not raw.endswith(b"\n"):
                open(MANIFEST, "ab").write(b"\r\n")
        with open(MANIFEST, "a", newline="", encoding="utf-8") as f:
            w = csv.writer(f)
            if not exists:
                w.writerow(HEADER)
            w.writerows(new_rows)
        print(f"Appended {len(new_rows)} rows to {MANIFEST}.")