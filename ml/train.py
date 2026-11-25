#!/usr/bin/env python3
"""Train and cross-validate the rule and decision tree; evaluate held-out data once."""
import argparse
import itertools
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path

import joblib
import numpy as np
import pandas as pd
import sklearn
from sklearn.metrics import (accuracy_score, confusion_matrix, f1_score,
                             precision_recall_fscore_support)
from sklearn.model_selection import GroupKFold
from sklearn.pipeline import make_pipeline
from sklearn.preprocessing import StandardScaler
from sklearn.tree import DecisionTreeClassifier, export_text
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import RandomForestClassifier

OUT = Path("ml/out")
TRAIN_PATH = OUT / "train_windows.csv"
TEST_PATH = OUT / "test_windows.csv"
FEATURES_PATH = OUT / "features.json"
CV_PATH = OUT / "cv_results.md"
RESULTS_PATH = OUT / "test_results.md"
SELECTION_PATH = OUT / "cv_selection.json"
LABELS = [0, 1, 2]
DEPTHS = [2, 3, 4, 5]
LEAVES = [1, 2, 3, 5]


def load_features():
    with FEATURES_PATH.open(encoding="utf-8") as f:
        return json.load(f)["features"]


def load_train(features):
    frame = pd.read_csv(TRAIN_PATH)
    if frame.empty:
        raise ValueError("training data is empty")
    return frame, frame[features].to_numpy(dtype=np.float64), frame["label"].to_numpy(dtype=int)


def rule_predict(frame, thresholds):
    rh_t, t_t, amb_t, prox_t = (thresholds[k] for k in ("RH_T", "T_T", "AMB_T", "PROX_T"))
    risk = (frame["rh_mean"].to_numpy() >= rh_t) | (frame["temp_mean"].to_numpy() >= t_t)
    access = (frame["amb_max"].to_numpy() >= amb_t) | (frame["prox_max"].to_numpy() >= prox_t)
    return np.where(risk, 2, np.where(access, 1, 0))


def fit_rule(frame, y):
    grids = (np.arange(80, 95.0001, .5), np.arange(31, 37.0001, .25),
             np.arange(5, 100.0001, 5), np.arange(40, 600.0001, 20))
    rh_grid, t_grid, amb_grid, prox_grid = grids
    rh = frame["rh_mean"].to_numpy()
    temp = frame["temp_mean"].to_numpy()
    amb = frame["amb_max"].to_numpy()
    prox = frame["prox_max"].to_numpy()

    # C-order flattening makes RH the outer, T the inner threshold index.
    m2 = ((rh[None, None, :] >= rh_grid[:, None, None]) |
          (temp[None, None, :] >= t_grid[None, :, None])).reshape(-1, len(y))
    # C-order flattening makes AMB the outer, PROX the inner threshold index.
    m1 = ((amb[None, None, :] >= amb_grid[:, None, None]) |
          (prox[None, None, :] >= prox_grid[None, :, None])).reshape(-1, len(y))

    m2 = m2.astype(np.int32)
    m1 = m1.astype(np.int32)
    y2 = (y == 2).astype(np.int32)
    y1_not2 = ((y == 1)[None, :] & ~m2.astype(bool)).astype(np.int32)
    y0_not2 = ((y == 0)[None, :] & ~m2.astype(bool)).astype(np.int32)
    c2 = m2 @ y2
    c1 = y1_not2 @ m1.T
    c0 = y0_not2 @ (~m1.astype(bool)).astype(np.int32).T
    total_correct = c2[:, None] + c1 + c0
    i2, i1 = np.unravel_index(np.argmax(total_correct), total_correct.shape)
    n_t, n_prox = len(t_grid), len(prox_grid)
    # The flattened pair index is outer_index * inner_count + inner_index.
    rh_i, t_i = divmod(i2, n_t)
    amb_i, prox_i = divmod(i1, n_prox)
    best = {"RH_T": float(rh_grid[rh_i]), "T_T": float(t_grid[t_i]),
            "AMB_T": float(amb_grid[amb_i]), "PROX_T": float(prox_grid[prox_i])}
    return best, float(total_correct[i2, i1] / len(y))


def cv_only():
    features = load_features()
    frame, _, y = load_train(features)  # Deliberately never opens TEST_PATH.
    groups = frame["session_id"].to_numpy()
    n_splits = len(np.unique(groups))
    if n_splits < 2:
        raise ValueError("GroupKFold requires at least two training sessions")
    absolute_features = [name for name in features if name not in ("temp_rel", "rh_rel")]
    relative_features = [name for name in absolute_features]
    relative_features[relative_features.index("temp_mean")] = "temp_rel"
    relative_features[relative_features.index("rh_mean")] = "rh_rel"
    absolute_x = frame[absolute_features].to_numpy(dtype=np.float64)
    has_baseline = frame[relative_features].notna().all(axis=1).to_numpy()
    relative_x = frame.loc[has_baseline, relative_features].to_numpy(dtype=np.float64)
    subset_abs_x = absolute_x[has_baseline]
    subset_y = y[has_baseline]
    subset_groups = groups[has_baseline]

    splitter = GroupKFold(n_splits=n_splits)
    folds = list(splitter.split(absolute_x, y, groups))

    base_true, base_pred = [], []
    rule_oof = np.empty(len(y), dtype=int)
    for train_i, val_i in folds:
        thresholds, _ = fit_rule(frame.iloc[train_i], y[train_i])
        fold_pred = rule_predict(frame.iloc[val_i], thresholds)
        base_true.extend(y[val_i])
        base_pred.extend(fold_pred)
        rule_oof[val_i] = fold_pred
    base_f1 = f1_score(base_true, base_pred, labels=LABELS, average="macro", zero_division=0)
    base_acc = accuracy_score(base_true, base_pred)

    model_scores = []
    tree_oof = {}
    for depth, leaf in itertools.product(DEPTHS, LEAVES):
        truth, pred = [], []
        oof_pred = np.empty(len(y), dtype=int)
        for train_i, val_i in folds:
            model = DecisionTreeClassifier(max_depth=depth, min_samples_leaf=leaf, random_state=0)
            model.fit(absolute_x[train_i], y[train_i])
            fold_pred = model.predict(absolute_x[val_i])
            truth.extend(y[val_i])
            pred.extend(fold_pred)
            oof_pred[val_i] = fold_pred
        model_scores.append({"depth": depth, "leaf": leaf,
                             "f1": f1_score(truth, pred, labels=LABELS, average="macro", zero_division=0),
                             "accuracy": accuracy_score(truth, pred)})
        tree_oof[(depth, leaf)] = oof_pred
    chosen = max(model_scores, key=lambda r: (r["f1"], -DEPTHS.index(r["depth"]), -LEAVES.index(r["leaf"])))

    def candidates():
        def logistic_model(c):
            try:
                classifier = LogisticRegression(C=c, solver="lbfgs", multi_class="multinomial", max_iter=2000)
            except TypeError:  # Newer sklearn versions use multinomial automatically for multiclass.
                classifier = LogisticRegression(C=c, solver="lbfgs", max_iter=2000)
            return make_pipeline(StandardScaler(), classifier)

        for depth, leaf in itertools.product(DEPTHS, LEAVES):
            yield f"DecisionTree depth={depth}, leaf={leaf}", lambda d=depth, l=leaf: DecisionTreeClassifier(max_depth=d, min_samples_leaf=l, random_state=0), "tree"
        for c in (0.01, 0.03, 0.1, 0.3, 1):
            yield f"LogisticRegression C={c}", lambda c=c: logistic_model(c), "logistic"
        for estimators, depth in itertools.product((10, 20), (3, 4)):
            yield f"RandomForest n_estimators={estimators}, depth={depth}", lambda n=estimators, d=depth: RandomForestClassifier(n_estimators=n, max_depth=d, random_state=0), "forest"

    def model_size(model, kind):
        if kind == "tree":
            return int(model.tree_.node_count)
        if kind == "forest":
            return int(sum(est.tree_.node_count for est in model.estimators_))
        classifier = model[-1]
        return int(classifier.coef_.size)

    candidate_rows = []
    for subset_name, subset_matrix, subset_labels, subset_group_values in (
            ("absolute", absolute_x, y, groups),
            ("absolute on baseline subset", subset_abs_x, subset_y, subset_groups),
            ("relative", relative_x, subset_y, subset_groups)):
        n_subset_splits = len(np.unique(subset_group_values))
        subset_folds = list(GroupKFold(n_splits=n_subset_splits).split(
            subset_matrix, subset_labels, subset_group_values))
        for candidate_name, factory, kind in candidates():
            truths, predictions = [], []
            oof = np.empty(len(subset_labels), dtype=int)
            for train_i, val_i in subset_folds:
                estimator = factory()
                estimator.fit(subset_matrix[train_i], subset_labels[train_i])
                truths.extend(subset_labels[val_i])
                fold_pred = estimator.predict(subset_matrix[val_i])
                predictions.extend(fold_pred)
                oof[val_i] = fold_pred
            final_estimator = factory().fit(subset_matrix, subset_labels)
            candidate_rows.append({"candidate": candidate_name, "features": subset_name,
                                   "kind": kind,
                                   "f1": f1_score(truths, predictions, labels=LABELS,
                                                   average="macro", zero_division=0),
                                   "accuracy": accuracy_score(truths, predictions),
                                   "size": model_size(final_estimator, kind),
                                   "oof": oof if subset_name == "absolute" else None,
                                   "size_kind": {"tree": "nodes", "forest": "total nodes",
                                                 "logistic": "coefficients"}[kind]})
    full_set_candidates = [row for row in candidate_rows if row["features"] == "absolute"]
    best_candidate = max(full_set_candidates, key=lambda row: row["f1"])
    selected_oof = best_candidate["oof"]
    selected_family = best_candidate["kind"]
    best_params = {}
    if selected_family == "tree":
        import re
        found = re.search(r"depth=(\d+), leaf=(\d+)", best_candidate["candidate"])
        best_params = {"max_depth": int(found.group(1)), "min_samples_leaf": int(found.group(2))}
    elif selected_family == "logistic":
        best_params = {"C": float(best_candidate["candidate"].split("=")[-1]), "max_iter": 2000}
    else:
        import re
        found = re.search(r"n_estimators=(\d+), depth=(\d+)", best_candidate["candidate"])
        best_params = {"n_estimators": int(found.group(1)), "max_depth": int(found.group(2)), "random_state": 0}
    best_tree = DecisionTreeClassifier(max_depth=chosen["depth"],
                                       min_samples_leaf=chosen["leaf"], random_state=0)
    best_tree.fit(absolute_x, y)
    best_tree_oof = tree_oof[(chosen["depth"], chosen["leaf"])]

    lines = ["# Cross-validation results", "", f"Baseline rule macro-F1: {base_f1:.6f}; accuracy: {base_acc:.6f}",
             "", "## Decision tree grid (absolute features)", "",
             "| max_depth | min_samples_leaf | macro-F1 | accuracy |", "|---:|---:|---:|---:|"]
    lines += [f"| {r['depth']} | {r['leaf']} | {r['f1']:.6f} | {r['accuracy']:.6f} |" for r in model_scores]
    lines += ["", f"Best tree setting: max_depth={chosen['depth']}, min_samples_leaf={chosen['leaf']}.",
              "", "## Model families and feature sets", "",
              "All scores use leave-one-session-out CV on training data. The absolute baseline-subset rows provide like-for-like comparisons with relative-feature rows.", "",
              "| Candidate | Feature set | Macro-F1 | Accuracy | Model size |", "|---|---|---:|---:|---:|"]
    lines += [f"| {r['candidate']} | {r['features']} | {r['f1']:.6f} | {r['accuracy']:.6f} | {r['size']} {r['size_kind']} |" for r in candidate_rows]
    lines += ["", "## Full-set candidates (selection set)", "",
              "Candidates here use all training windows and the 11 absolute features. The baseline rule is reported separately and is never selected as a learned model.", "",
              "| Candidate | Macro-F1 | Accuracy | Model size |", "|---|---:|---:|---:|"]
    lines += [f"| {r['candidate']} | {r['f1']:.6f} | {r['accuracy']:.6f} | {r['size']} {r['size_kind']} |" for r in full_set_candidates]
    lines += ["", "## Relative vs absolute features (same subset, diagnostic only)", "",
              "These baseline-subset comparisons are diagnostic only and are excluded from model selection.", "",
              "| Candidate | Feature set | Macro-F1 | Accuracy | Model size |", "|---|---|---:|---:|---:|"]
    diagnostics = [r for r in candidate_rows if r["features"] != "absolute"]
    lines += [f"| {r['candidate']} | {r['features']} | {r['f1']:.6f} | {r['accuracy']:.6f} | {r['size']} {r['size_kind']} |" for r in diagnostics]
    lines += ["", f"Best learned candidate: {best_candidate['candidate']} (macro-F1={best_candidate['f1']:.6f}, accuracy={best_candidate['accuracy']:.6f}); selected on full training data only.",
              "", "## Per-session leave-one-session-out results", "",
              "| Session | Label | Windows | Rule correct | Best-tree correct | Selected-model correct |", "|---|---:|---:|---:|---:|---:|"]
    session_results = []
    for session in sorted(frame["session_id"].unique()):
        idx = np.flatnonzero(groups == session)
        labels = sorted(np.unique(y[idx]))
        label = labels[0] if len(labels) == 1 else ",".join(map(str, labels))
        rule_ok = int(np.sum(rule_oof[idx] == y[idx]))
        tree_ok = int(np.sum(best_tree_oof[idx] == y[idx]))
        selected_ok = int(np.sum(selected_oof[idx] == y[idx]))
        session_results.append({"session": session, "label": label, "windows": len(idx),
                                "rule_correct": rule_ok, "tree_correct": tree_ok,
                                "selected_correct": selected_ok})
        lines.append(f"| {session} | {label} | {len(idx)} | {rule_ok} | {tree_ok} | {selected_ok} |")
    for title, prediction in (("Rule", rule_oof), ("Selected model", selected_oof)):
        cm = confusion_matrix(y, prediction, labels=LABELS)
        precision, recall, _, _ = precision_recall_fscore_support(y, prediction, labels=LABELS, zero_division=0)
        lines += ["", f"## Pooled LOSO confusion matrix: {title}", "",
                  "Rows=true, columns=predicted (states 0, 1, 2).", "",
                  "| True \\ Predicted | 0 | 1 | 2 |", "|---:|---:|---:|---:|"]
        lines += [f"| {state} | " + " | ".join(map(str, cm[state])) + " |" for state in LABELS]
        lines += ["", "| State | Precision | Recall |", "|---:|---:|---:|"]
        lines += [f"| {state} | {precision[state]:.6f} | {recall[state]:.6f} |" for state in LABELS]
    lines += ["", "## Chosen tree fitted on all training data", "", "```text",
              export_text(best_tree, feature_names=absolute_features).rstrip(), "```", "",
              "<!-- cv_metadata: " + json.dumps({"chosen": {"max_depth": chosen["depth"],
                                                              "min_samples_leaf": chosen["leaf"]}}) + " -->", ""]
    OUT.mkdir(parents=True, exist_ok=True)
    CV_PATH.write_text("\n".join(lines), encoding="utf-8")
    SELECTION_PATH.write_text(json.dumps({"selected_model": {"family": selected_family,
                                                               "hyperparameters": best_params},
                                         "selected_cv_macro_f1": best_candidate["f1"],
                                         "best_tree": {"max_depth": chosen["depth"],
                                                       "min_samples_leaf": chosen["leaf"]},
                                         "rule_cv_macro_f1": base_f1,
                                         "feature_order": absolute_features}, indent=2) + "\n", encoding="utf-8")
    print(f"Baseline rule CV macro-F1={base_f1:.6f}, accuracy={base_acc:.6f}")
    for r in model_scores:
        print(f"max_depth={r['depth']} min_samples_leaf={r['leaf']}: macro-F1={r['f1']:.6f}, accuracy={r['accuracy']:.6f}")
    print(f"Chosen tree: max_depth={chosen['depth']}, min_samples_leaf={chosen['leaf']}")
    for r in full_set_candidates:
        print(f"{r['candidate']} [{r['features']}]: macro-F1={r['f1']:.6f}, accuracy={r['accuracy']:.6f}, model_size={r['size']} {r['size_kind']}")
    print("Relative vs absolute features (same subset, diagnostic only):")
    for r in diagnostics:
        print(f"{r['candidate']} [{r['features']}]: macro-F1={r['f1']:.6f}, accuracy={r['accuracy']:.6f}, model_size={r['size']} {r['size_kind']}")
    print(f"Best learned candidate: {best_candidate['candidate']}, macro-F1={best_candidate['f1']:.6f}, accuracy={best_candidate['accuracy']:.6f}. Selected on full training data only.")
    print("Per-session leave-one-session-out results:")
    print("Session | Label | Windows | Rule correct | Best-tree correct | Selected-model correct")
    for r in session_results:
        print(f"{r['session']} | {r['label']} | {r['windows']} | {r['rule_correct']} | {r['tree_correct']} | {r['selected_correct']}")
    for title, prediction in (("Rule", rule_oof), ("Selected model", selected_oof)):
        print(f"LOSO confusion matrix ({title}; rows=true, columns=predicted):")
        print(confusion_matrix(y, prediction, labels=LABELS))
    print(export_text(best_tree, feature_names=absolute_features))


def final(force):
    if RESULTS_PATH.exists() and not force:
        raise SystemExit("Refusing final evaluation: ml/out/test_results.md already exists. Use --force-reeval to re-evaluate.")
    if force:
        print("WARNING: re-evaluating invalidates the held-out claim.")
    if not SELECTION_PATH.exists():
        raise SystemExit("Run --cv-only successfully before --final; ml/out/cv_selection.json is missing.")
    selection = json.loads(SELECTION_PATH.read_text(encoding="utf-8"))
    features = load_features()
    absolute_features = [name for name in features if name not in ("temp_rel", "rh_rel")]
    frame, x, y = load_train(absolute_features)
    thresholds, _ = fit_rule(frame, y)
    tree_params = selection["best_tree"]
    tree = DecisionTreeClassifier(**tree_params, random_state=0).fit(x, y)
    chosen = selection["selected_model"]
    family, params = chosen["family"], chosen["hyperparameters"]
    if family == "tree":
        selected = DecisionTreeClassifier(**params, random_state=0)
    elif family == "logistic":
        try:
            classifier = LogisticRegression(**params, solver="lbfgs", multi_class="multinomial")
        except TypeError:
            classifier = LogisticRegression(**params, solver="lbfgs")
        selected = make_pipeline(StandardScaler(), classifier)
    else:
        selected = RandomForestClassifier(**params)
    selected.fit(x, y)
    frozen = {"features": absolute_features, "selected_model": chosen,
              "best_tree": tree_params, "rule_thresholds": thresholds,
              "git_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
              "timestamp_utc": datetime.now(timezone.utc).isoformat(),
              "sklearn_version": sklearn.__version__, "numpy_version": np.__version__}
    joblib.dump({"selected_model": selected, "tree": tree, "features": absolute_features}, OUT / "model.joblib")
    (OUT / "frozen.json").write_text(json.dumps(frozen, indent=2) + "\n", encoding="utf-8")

    # First access to held-out data occurs only after model and metadata are frozen.
    test = pd.read_csv(TEST_PATH)
    truth = test["label"].to_numpy(dtype=int)
    test_x = test[absolute_features].to_numpy(dtype=np.float64)
    outputs = [("Rule", rule_predict(test, thresholds)), ("Tree", tree.predict(test_x)),
               ("Selected model", selected.predict(test_x))]
    report = ["# Held-out test results", "", f"Test window count: {len(test)}", ""]
    for title, pred in outputs:
        cm = confusion_matrix(truth, pred, labels=LABELS)
        precision, recall, f1, _ = precision_recall_fscore_support(
            truth, pred, labels=LABELS, zero_division=0)
        report += [f"## {title}", "", "Confusion matrix (rows=true, columns=predicted; states 0, 1, 2):", "",
                   "| True \\ Predicted | 0 | 1 | 2 |", "|---:|---:|---:|---:|"]
        report += [f"| {state} | " + " | ".join(map(str, cm[state])) + " |" for state in LABELS]
        report += ["", "| State | Precision | Recall | F1 |", "|---:|---:|---:|---:|"]
        report += [f"| {state} | {precision[state]:.6f} | {recall[state]:.6f} | {f1[state]:.6f} |" for state in LABELS]
        report += ["", f"Overall accuracy: {accuracy_score(truth, pred):.6f}", "", "Misclassified windows:", ""]
        wrong = np.flatnonzero(truth != pred)
        if len(wrong):
            report += ["| session_id | window | true label | predicted label |", "|---|---:|---:|---:|"]
            report += [f"| {test.iloc[i]['session_id']} | {test.iloc[i]['window']} | {truth[i]} | {pred[i]} |" for i in wrong]
        else:
            report.append("None.")
        report.append("")
    RESULTS_PATH.write_text("\n".join(report), encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--cv-only", action="store_true", help="cross-validate using training data only (default)")
    mode.add_argument("--final", action="store_true", help="freeze and evaluate on test data")
    parser.add_argument("--force-reeval", action="store_true")
    args = parser.parse_args()
    if args.force_reeval and not args.final:
        parser.error("--force-reeval requires --final")
    if args.final:
        final(args.force_reeval)
    else:
        cv_only()
