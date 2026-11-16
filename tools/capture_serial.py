#!/usr/bin/env python3
"""Record labeled DATA lines from the RT-Spark firmware logger into a CSV file.

Firmware protocol (src/main.c ConsoleTask):
  send  l=<session>,<label>   -> board replies  MODE,logger,session=...,label=...
  board then prints            DATA,session,label,seq,tick_ms,temp_c,rh_pct,
                               ambient_raw,proximity_raw,valid_mask,buttons
  send  x                     -> board replies  MODE,stopped

Every line received during recording (BOOT, SENSOR, IMU, RTC, ...) is also saved
unmodified to data/transcripts/<session>.log; the CSV holds only valid DATA rows.

Examples:
  python capture_serial.py --list-ports
  python capture_serial.py --port COM5 --session train_s0_1 --label 0 --seconds 62
"""
import argparse
import csv
import datetime as dt
import pathlib
import re
import sys
import time

import serial
from serial.tools import list_ports

HEADER = ["session_id", "label", "sequence", "tick_ms", "temp_c", "rh_pct",
          "ambient_raw", "proximity_raw", "valid_mask", "buttons"]
MANIFEST_HEADER = ["session_id", "label", "start_utc", "seconds_requested",
                   "rows_kept", "rows_with_NA", "rows_skipped_warmup",
                   "bad_lines", "notes"]
SESSION_RE = re.compile(r"^[A-Za-z0-9_\-]{1,23}$")


def read_line(ser):
    return ser.readline().decode("ascii", "replace").strip()


def send_slow(ser, text, gap=0.04):
    # The firmware polls the UART roughly every 20 ms, so send one character
    # at a time to avoid dropped bytes.
    for ch in text:
        ser.write(ch.encode("ascii"))
        ser.flush()
        time.sleep(gap)


def wait_for(ser, token, timeout):
    end = time.time() + timeout
    while time.time() < end:
        if token in read_line(ser):
            return True
    return False


def start_logger(ser, session, label, tries=3):
    for _ in range(tries):
        send_slow(ser, "\r")                      # flush any half-typed command
        time.sleep(0.1)
        ser.reset_input_buffer()
        send_slow(ser, f"l={session},{label}\r")
        if wait_for(ser, f"MODE,logger,session={session}", 3.0):
            return True
    return False


def stop_logger(ser):
    for _ in range(3):
        send_slow(ser, "x\r")
        if wait_for(ser, "MODE,stopped", 2.0):
            return True
    return False


def append_manifest(path, row):
    new = not path.exists()
    with path.open("a", newline="") as f:
        w = csv.writer(f)
        if new:
            w.writerow(MANIFEST_HEADER)
        w.writerow(row)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--list-ports", action="store_true")
    ap.add_argument("--port")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--session", help="1-23 chars: letters, digits, _ or -")
    ap.add_argument("--label", type=int, choices=[0, 1, 2, 3])
    ap.add_argument("--seconds", type=float, default=62.0)
    ap.add_argument("--skip", type=int, default=2,
                    help="rows to drop at the start of a session (settling)")
    ap.add_argument("--out-dir", default="data/raw")
    ap.add_argument("--notes", default="")
    ap.add_argument("--force", action="store_true", help="overwrite an existing session file")
    args = ap.parse_args()

    if args.list_ports:
        for p in list_ports.comports():
            print(p.device, "-", p.description)
        return 0
    if not (args.port and args.session is not None and args.label is not None):
        ap.error("--port, --session and --label are required")
    if not SESSION_RE.match(args.session):
        ap.error("session must be 1-23 characters of letters, digits, _ or -")

    out_dir = pathlib.Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / f"{args.session}.csv"
    transcript_dir = out_dir.parent / "transcripts"
    transcript_dir.mkdir(parents=True, exist_ok=True)
    transcript_path = transcript_dir / f"{args.session}.log"
    if out_path.exists() and not args.force:
        print(f"Refusing to overwrite {out_path} (use --force).")
        return 2

    start_utc = dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds")
    rows, na_rows, skipped, bad = [], 0, 0, 0

    with serial.Serial(args.port, args.baud, timeout=1.0) as ser:
        if not start_logger(ser, args.session, args.label):
            print("Board did not confirm logger mode. Is the monitor closed and the board running?")
            return 1
        print(f"Recording session={args.session} label={args.label} for {args.seconds:.0f} s ...")
        end = time.time() + args.seconds
        transcript = transcript_path.open("w", encoding="utf-8", newline="")
        while time.time() < end:
            line = read_line(ser)
            if line:
                transcript.write(line + "\n")      # raw, unfiltered record
            if not line.startswith("DATA,"):
                continue
            f = line.split(",")
            if len(f) != 11:
                bad += 1
                continue
            f = f[1:]                                   # drop the "DATA" tag
            if f[0] != args.session or f[1] != str(args.label):
                bad += 1
                continue
            if skipped < args.skip:
                skipped += 1
                continue
            if "NA" in (f[4], f[5]):
                na_rows += 1
            rows.append(f)
            if len(rows) % 10 == 0:
                print(f"  {len(rows)} rows")
        stop_logger(ser)
        transcript.close()

    with out_path.open("w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(HEADER)
        w.writerows(rows)

    append_manifest(out_dir.parent / "manifest.csv",
                    [args.session, args.label, start_utc, args.seconds,
                     len(rows), na_rows, skipped, bad, args.notes])
    print(f"Saved {len(rows)} rows ({na_rows} with NA, {bad} bad lines) to {out_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
