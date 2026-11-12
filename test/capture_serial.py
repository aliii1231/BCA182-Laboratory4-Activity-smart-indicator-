#!/usr/bin/env python3
"""Capture board logger serial output into an unmodified, session-specific file."""
import argparse, datetime, pathlib, sys
try:
    import serial
except ImportError as exc:
    raise SystemExit("Install pyserial: python -m pip install pyserial") from exc

def main():
    p=argparse.ArgumentParser();p.add_argument("port");p.add_argument("session_id");p.add_argument("label",type=int,choices=range(4));p.add_argument("--baud",type=int,default=115200);p.add_argument("--out",type=pathlib.Path,default=pathlib.Path("data/raw"));a=p.parse_args()
    a.out.mkdir(parents=True,exist_ok=True); stamp=datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ"); path=a.out/f"{a.session_id}_{a.label}_{stamp}.csv"
    with serial.Serial(a.port,a.baud,timeout=1) as ser, path.open("w",encoding="utf-8",newline="") as f:
        ser.reset_input_buffer(); ser.write(f"l={a.session_id},{a.label}\r\n".encode()); print(f"Capturing to {path}; Ctrl-C stops capture.")
        try:
            while True:
                line=ser.readline()
                if line:
                    decoded=line.decode("utf-8",errors="replace").rstrip("\r\n"); print(decoded); f.write(decoded+"\n");f.flush()
        except KeyboardInterrupt:
            ser.write(b"d\r\n"); print(f"\nSaved serial transcript: {path}")
if __name__=="__main__":main()
