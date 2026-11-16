import csv, glob, os, statistics as st

def col(rows, k):
    return [float(r[k]) for r in rows if r[k] != "NA"]

def slope_per_min(ys):
    n = len(ys); xs = list(range(n))      # about 1 sample per second
    mx = sum(xs) / n; my = sum(ys) / n
    num = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    den = sum((x - mx) ** 2 for x in xs)
    return num / den * 60

for f in sorted(glob.glob("data/raw/*.csv")):
    name = os.path.basename(f)
    rows = list(csv.DictReader(open(f)))
    if not rows:
        print(name, "no rows"); continue
    print(name, len(rows), "rows")
    for k in ("temp_c", "rh_pct", "ambient_raw", "proximity_raw"):
        v = col(rows, k)
        print(f"  {k:14s} min {min(v):8.3f}  mean {st.mean(v):8.3f}  max {max(v):8.3f}")
    if name.startswith("settle"):
        t = col(rows, "temp_c")[-300:]
        print(f"  temp drift over last {len(t)} s: {slope_per_min(t):+.3f} C/min")