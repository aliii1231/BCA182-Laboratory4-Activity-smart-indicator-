#!/usr/bin/env python3
"""Train/evaluate a compact CART model from RT-Spark CSV captures.

Input is a normalized CSV containing one row per 10-second window and columns:
session_id, split (train/test), label, then sample columns prefixed with
temp_c_, rh_pct_, ambient_raw_, proximity_raw_ (e.g. temp_c_0..temp_c_9).
The capture serial stream contains raw samples; window assembly is explicit
in this script to preserve recording-session split boundaries.
"""
from __future__ import annotations
import argparse, csv, json, math
from collections import Counter, defaultdict
from pathlib import Path

FEATURES = ("temp_c", "rh_pct", "ambient_raw", "proximity_raw")
CLASSES = (0, 1, 2, 3)

def load_rows(path: Path):
    rows, excluded = [], []
    with path.open(newline="", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)
        required = {"session_id", "split", "label"}
        if not reader.fieldnames or not required.issubset(reader.fieldnames):
            raise ValueError("input requires session_id, split, label and sample feature columns")
        for line, row in enumerate(reader, 2):
            try:
                label = int(row["label"])
                if label not in CLASSES or row["split"] not in ("train", "test") or not row["session_id"].strip():
                    raise ValueError("invalid label/split/session")
                samples = {}
                for feature in FEATURES:
                    cols = sorted((c for c in row if c.startswith(feature + "_")), key=lambda x: int(x.rsplit("_", 1)[1]))
                    if not cols: raise ValueError("missing " + feature + " samples")
                    values = [float(row[c]) for c in cols]
                    if not all(math.isfinite(x) for x in values): raise ValueError("non-finite reading")
                    samples[feature] = values
                if len({len(v) for v in samples.values()}) != 1: raise ValueError("unequal window lengths")
                # Conservative sensor-range cleaning. Invalid windows are excluded, never imputed.
                if any(not -40 <= x <= 85 for x in samples["temp_c"]): raise ValueError("temperature out of range")
                if any(not 0 <= x <= 100 for x in samples["rh_pct"]): raise ValueError("humidity out of range")
                if any(not 0 <= x <= 16383 for x in samples["ambient_raw"]): raise ValueError("ambient raw out of range")
                if any(not 0 <= x <= 1023 for x in samples["proximity_raw"]): raise ValueError("proximity raw out of range")
                rows.append({"session": row["session_id"], "split": row["split"], "label": label, "samples": samples})
            except (ValueError, KeyError) as e:
                excluded.append({"line": line, "reason": str(e)})
    return rows, excluded

def extract(samples):
    out = []
    for name in FEATURES:
        xs = samples[name]; n = len(xs); mean = sum(xs) / n
        var = sum((x - mean) ** 2 for x in xs) / n
        slope = sum((i - (n - 1) / 2) * (x - mean) for i, x in enumerate(xs)) / (sum((i - (n - 1) / 2) ** 2 for i in range(n)) or 1)
        out.extend((mean, math.sqrt(var), min(xs), max(xs), slope))
    return out

def baseline(x):
    # Transparent demonstration baseline; calibrate operational thresholds to use case.
    temp, rh, light, prox = x[0], x[5], x[10], x[15]
    if temp >= 30 or rh >= 75 or light >= 1000: return 2
    if prox >= 100 or light >= 300: return 1
    return 0

def metrics(y_true, y_pred):
    cm = [[sum(1 for a,b in zip(y_true,y_pred) if a==i and b==j) for j in CLASSES] for i in CLASSES]
    per = {}
    for i in CLASSES:
        tp=cm[i][i]; fp=sum(cm[r][i] for r in CLASSES if r!=i); fn=sum(cm[i][c] for c in CLASSES if c!=i)
        per[str(i)]={"tp":tp,"fp":fp,"fn":fn,"precision_exact":f"{tp}/{tp+fp}" if tp+fp else "undefined","recall_exact":f"{tp}/{tp+fn}" if tp+fn else "undefined","precision":tp/(tp+fp) if tp+fp else None,"recall":tp/(tp+fn) if tp+fn else None}
    return cm,per

def train_cart(X,y,max_depth=4):
    """Small deterministic CART (Gini), exporting nested nodes to C."""
    def gini(ids):
        counts=Counter(y[i] for i in ids); n=len(ids)
        return 1-sum((v/n)**2 for v in counts.values()) if n else 0
    def build(ids,depth):
        counts=Counter(y[i] for i in ids); pred=min(counts,key=lambda c:(-counts[c],c))
        node={"class":pred}
        if depth>=max_depth or len(counts)==1 or len(ids)<4:return node
        base=gini(ids); best=None
        for f in range(len(X[0])):
            vals=sorted(set(X[i][f] for i in ids))
            for lo,hi in zip(vals,vals[1:]):
                t=(lo+hi)/2; left=[i for i in ids if X[i][f]<=t]; right=[i for i in ids if X[i][f]>t]
                gain=base-(len(left)*gini(left)+len(right)*gini(right))/len(ids)
                cand=(gain,-f,-t,f,t,left,right)
                if left and right and (best is None or cand[:3]>best[:3]):best=cand
        if best is None or best[0]<=1e-12:return node
        _,_,_,f,t,left,right=best
        node.update(feature=f,threshold=t,left=build(left,depth+1),right=build(right,depth+1))
        return node
    return build(list(range(len(y))),0)

def predict(node,x):
    while "feature" in node: node=node["left"] if x[node["feature"]]<=node["threshold"] else node["right"]
    return node["class"]

def emit_c(tree, header: Path, source: Path):
    nodes=[]
    def flatten(n):
        idx=len(nodes); nodes.append(None)
        if "feature" in n: left=flatten(n["left"]); right=flatten(n["right"]); nodes[idx]=(n["feature"],n["threshold"],left,right,-1)
        else:nodes[idx]=(-1,0.0,-1,-1,n["class"])
        return idx
    flatten(tree)
    h='''#ifndef MODEL_WEIGHTS_H\n#define MODEL_WEIGHTS_H\n#include <stdint.h>\n#define MODEL_FEATURE_COUNT 20\n#define MODEL_NODE_COUNT %d\nint model_predict(const float features[MODEL_FEATURE_COUNT]);\n#endif\n'''%len(nodes)
    c=['#include "model_weights.h"','typedef struct { int16_t feature; float threshold; int16_t left,right; int8_t klass; } node_t;','static const node_t nodes[MODEL_NODE_COUNT] = {']
    for f,t,l,r,k in nodes:c.append(' {%d, %.9gf, %d, %d, %d},'%(f,t,l,r,k))
    c += ['};','int model_predict(const float x[MODEL_FEATURE_COUNT]) { int i=0; while(nodes[i].feature>=0) i=(x[nodes[i].feature]<=nodes[i].threshold)?nodes[i].left:nodes[i].right; return nodes[i].klass; }']
    header.write_text(h,encoding="utf-8"); source.write_text("\n".join(c)+"\n",encoding="utf-8")

def main():
    ap=argparse.ArgumentParser(); ap.add_argument("csv",type=Path); ap.add_argument("--out",type=Path,default=Path("build/ml")); a=ap.parse_args()
    rows,excluded=load_rows(a.csv); train=[r for r in rows if r['split']=='train']; test=[r for r in rows if r['split']=='test']
    if set(r['session'] for r in train)&set(r['session'] for r in test): raise SystemExit("ERROR: train/test session leakage")
    for cls in CLASSES:
        nt=sum(r['label']==cls for r in train); ne=sum(r['label']==cls for r in test)
        if nt<30 or ne<10: raise SystemExit(f"ERROR: class {cls} requires >=30 train and >=10 held-out test windows; found {nt}/{ne}")
    Xtr=[extract(r['samples']) for r in train]; ytr=[r['label'] for r in train]; Xte=[extract(r['samples']) for r in test]; yte=[r['label'] for r in test]
    tree=train_cart(Xtr,ytr); pred=[predict(tree,x) for x in Xte]; base=[baseline(x) for x in Xte]
    a.out.mkdir(parents=True,exist_ok=True); emit_c(tree,a.out/'model_weights.h',a.out/'model_inference.c')
    cm,per=metrics(yte,pred); bcm,bper=metrics(yte,base)
    report={"samples":{"train":len(train),"test":len(test),"excluded":excluded,"train_labels":dict(Counter(ytr)),"test_labels":dict(Counter(yte))},"model":{"confusion_matrix_rows_actual_cols_predicted":cm,"per_class":per,"tree":tree},"baseline":{"confusion_matrix_rows_actual_cols_predicted":bcm,"per_class":bper},"formula":"precision=TP/(TP+FP); recall=TP/(TP+FN); denominators and integer counts retained for exact unrounded values"}
    (a.out/'evaluation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2)); print(f"Exported {a.out/'model_weights.h'} and {a.out/'model_inference.c'}")

if __name__=='__main__':main()
