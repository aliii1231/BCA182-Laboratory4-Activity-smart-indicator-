# Dataset collection protocol and manifest

## Status

No physical board data has been collected or included. Capture real data from the RT-Spark logger. Do not label simulated/example readings as hardware observations.

## Logger format and sampling

- Sampling frequency: 1 Hz nominal.
- Window duration: 10 seconds / 10 successive samples. Collect complete non-overlapping windows by session.
- Console format: CSV records prefixed by `DATA,session,label,...`; command `l` enters logger mode, `d` returns to diagnostic mode.
- Fields: `sequence`, `tick_ms`, `temp_c`, `rh_pct`, `ambient_raw`, `proximity_raw`, `valid_mask`, `buttons`. Temperature is °C, humidity is %RH, ambient and proximity are raw AP3216C counts. `valid_mask` bit 0 is temperature/humidity, bit 1 is ambient/proximity.
- Start logger mode with `l=<session_id>,<label>` (for example `l=S01,0`), where the session identifier is anonymized. Never infer labels from the measurements after capture.

## Labels

| Label | Definition |
|---|---|
| `0` | Sealed/closed storage, exposure within configured prototype thresholds |
| `1` | Active access/open condition, controlled access cue present |
| `2` | Adverse exposure condition, controlled sustained environmental excursion |
| `3` | Fault condition; invalid sensor/fault scenarios, reserved for system diagnostics |

Collect balanced examples, with at least **30 labeled training windows and 10 held-out test windows per class**. Training and test sets must come from separate recording sessions on separate capture occasions, with zero overlapping or near-duplicate windows. Do not randomly split contiguous windows from one recording.

## Reproducible procedure

1. Record board revision, firmware commit, sensor inventory, date, session ID, sampling rate and the physical setup. Do not record names, household identifiers, or sensitive stored-content information.
2. Open a serial terminal at 115200 8-N-1 on the board's USART1 virtual COM port. Run `python test/capture_serial.py COMx SESSION_ID LABEL` (replace `COMx` with the actual port). The script sends `l=<session_id>,<label>` and saves an unmodified transcript under ignored `data/raw/`; verify the 1 Hz monotonic `DATA` rows. Capture to a new file per session.
3. For each capture session, establish one controlled condition at a time and mark its label in the manifest. Allow the physical condition to stabilize before capturing. Keep test recordings out of training/tuning and do not copy a recording across splits.
4. Capture at least 10 seconds per window. Keep only windows with all required sensor validity bits set for classes 0–2. For class 3, intentionally document the fault and expected validity pattern.
5. Stop with `d`, save the unmodified serial capture in an untracked/private raw-data location, then create an anonymized manifest row with the recording filename, session, split, label, start/end, valid sample count, and notes about environmental variation.
6. Check counts per class and session separation before training. The ML script rejects malformed rows and reports excluded records; it must not silently interpolate missing physical measurements.

## Manifest template

```csv
recording_id,session_id,split,label,start_utc,end_utc,window_count,valid_count,environment_notes,firmware_commit
[RECORDING_ID],[SESSION_ID],train,[0-3],[HARDWARE_MEASUREMENT_REQUIRED],[HARDWARE_MEASUREMENT_REQUIRED],0,0,[ANONYMIZED_NOTES],[COMMIT]
```

## Limitations

Thresholds and classes are experimental and are not safety certifications. Raw ambient/proximity counts may depend on enclosure geometry, distance, component tolerance, and lighting. Keep test sessions separate and report class imbalance and conditions. Collect actual measurements only on the target board.
