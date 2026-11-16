# Task 3 serial capture

Run `tools/capture_serial.py` from the repository root with the serial monitor closed, for example:

```powershell
python tools/capture_serial.py --port COM5 --session train_s0_1 --label 0 --seconds 62 --notes "backlight=on dark_static box=A"
```

It writes valid DATA rows to `data/raw/<session>.csv`, every received line unmodified to `data/transcripts/<session>.log`, and appends a row to `data/manifest.csv`.

Install pyserial with `python -m pip install pyserial`. `test/capture_serial.py` is an older raw-transcript tool and is superseded by this one.
