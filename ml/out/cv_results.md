# Cross-validation results

Baseline rule macro-F1: 0.874563; accuracy: 0.872549

## Decision tree grid (absolute features)

| max_depth | min_samples_leaf | macro-F1 | accuracy |
|---:|---:|---:|---:|
| 2 | 1 | 0.775428 | 0.774510 |
| 2 | 2 | 0.775428 | 0.774510 |
| 2 | 3 | 0.775428 | 0.774510 |
| 2 | 5 | 0.775428 | 0.774510 |
| 3 | 1 | 0.775428 | 0.774510 |
| 3 | 2 | 0.775428 | 0.774510 |
| 3 | 3 | 0.775428 | 0.774510 |
| 3 | 5 | 0.775428 | 0.774510 |
| 4 | 1 | 0.775428 | 0.774510 |
| 4 | 2 | 0.775428 | 0.774510 |
| 4 | 3 | 0.775428 | 0.774510 |
| 4 | 5 | 0.775428 | 0.774510 |
| 5 | 1 | 0.775428 | 0.774510 |
| 5 | 2 | 0.775428 | 0.774510 |
| 5 | 3 | 0.775428 | 0.774510 |
| 5 | 5 | 0.775428 | 0.774510 |

Best tree setting: max_depth=2, min_samples_leaf=1.

## Model families and feature sets

All scores use leave-one-session-out CV on training data. The absolute baseline-subset rows provide like-for-like comparisons with relative-feature rows.

| Candidate | Feature set | Macro-F1 | Accuracy | Model size |
|---|---|---:|---:|---:|
| DecisionTree depth=2, leaf=1 | absolute | 0.775428 | 0.774510 | 5 nodes |
| DecisionTree depth=2, leaf=2 | absolute | 0.775428 | 0.774510 | 5 nodes |
| DecisionTree depth=2, leaf=3 | absolute | 0.775428 | 0.774510 | 5 nodes |
| DecisionTree depth=2, leaf=5 | absolute | 0.775428 | 0.774510 | 5 nodes |
| DecisionTree depth=3, leaf=1 | absolute | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=3, leaf=2 | absolute | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=3, leaf=3 | absolute | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=3, leaf=5 | absolute | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=4, leaf=1 | absolute | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=4, leaf=2 | absolute | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=4, leaf=3 | absolute | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=4, leaf=5 | absolute | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=5, leaf=1 | absolute | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=5, leaf=2 | absolute | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=5, leaf=3 | absolute | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=5, leaf=5 | absolute | 0.775428 | 0.774510 | 7 nodes |
| LogisticRegression C=0.01 | absolute | 0.786325 | 0.794118 | 33 coefficients |
| LogisticRegression C=0.03 | absolute | 0.912323 | 0.911765 | 33 coefficients |
| LogisticRegression C=0.1 | absolute | 0.856523 | 0.852941 | 33 coefficients |
| LogisticRegression C=0.3 | absolute | 0.783104 | 0.784314 | 33 coefficients |
| LogisticRegression C=1 | absolute | 0.740334 | 0.735294 | 33 coefficients |
| RandomForest n_estimators=10, depth=3 | absolute | 0.749550 | 0.745098 | 80 total nodes |
| RandomForest n_estimators=10, depth=4 | absolute | 0.749550 | 0.745098 | 90 total nodes |
| RandomForest n_estimators=20, depth=3 | absolute | 0.820873 | 0.813725 | 166 total nodes |
| RandomForest n_estimators=20, depth=4 | absolute | 0.830203 | 0.823529 | 198 total nodes |
| DecisionTree depth=2, leaf=1 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=2, leaf=2 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=2, leaf=3 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=2, leaf=5 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=3, leaf=1 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=3, leaf=2 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=3, leaf=3 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=3, leaf=5 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=4, leaf=1 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=4, leaf=2 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=4, leaf=3 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=4, leaf=5 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=5, leaf=1 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=5, leaf=2 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=5, leaf=3 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=5, leaf=5 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| LogisticRegression C=0.01 | absolute on baseline subset | 0.557078 | 0.726190 | 33 coefficients |
| LogisticRegression C=0.03 | absolute on baseline subset | 0.721492 | 0.785714 | 33 coefficients |
| LogisticRegression C=0.1 | absolute on baseline subset | 0.802873 | 0.833333 | 33 coefficients |
| LogisticRegression C=0.3 | absolute on baseline subset | 0.802873 | 0.833333 | 33 coefficients |
| LogisticRegression C=1 | absolute on baseline subset | 0.802873 | 0.833333 | 33 coefficients |
| RandomForest n_estimators=10, depth=3 | absolute on baseline subset | 0.827002 | 0.857143 | 68 total nodes |
| RandomForest n_estimators=10, depth=4 | absolute on baseline subset | 0.862418 | 0.880952 | 82 total nodes |
| RandomForest n_estimators=20, depth=3 | absolute on baseline subset | 0.851753 | 0.880952 | 138 total nodes |
| RandomForest n_estimators=20, depth=4 | absolute on baseline subset | 0.827002 | 0.857143 | 170 total nodes |
| DecisionTree depth=2, leaf=1 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=2, leaf=2 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=2, leaf=3 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=2, leaf=5 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=3, leaf=1 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=3, leaf=2 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=3, leaf=3 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=3, leaf=5 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=4, leaf=1 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=4, leaf=2 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=4, leaf=3 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=4, leaf=5 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=5, leaf=1 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=5, leaf=2 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=5, leaf=3 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=5, leaf=5 | relative | 0.889905 | 0.916667 | 5 nodes |
| LogisticRegression C=0.01 | relative | 0.574253 | 0.750000 | 33 coefficients |
| LogisticRegression C=0.03 | relative | 0.546622 | 0.690476 | 33 coefficients |
| LogisticRegression C=0.1 | relative | 0.763535 | 0.809524 | 33 coefficients |
| LogisticRegression C=0.3 | relative | 0.815874 | 0.845238 | 33 coefficients |
| LogisticRegression C=1 | relative | 0.815874 | 0.845238 | 33 coefficients |
| RandomForest n_estimators=10, depth=3 | relative | 0.864461 | 0.892857 | 70 total nodes |
| RandomForest n_estimators=10, depth=4 | relative | 0.827002 | 0.857143 | 86 total nodes |
| RandomForest n_estimators=20, depth=3 | relative | 0.864461 | 0.892857 | 140 total nodes |
| RandomForest n_estimators=20, depth=4 | relative | 0.864461 | 0.892857 | 174 total nodes |

## Full-set candidates (selection set)

Candidates here use all training windows and the 11 absolute features. The baseline rule is reported separately and is never selected as a learned model.

| Candidate | Macro-F1 | Accuracy | Model size |
|---|---:|---:|---:|
| DecisionTree depth=2, leaf=1 | 0.775428 | 0.774510 | 5 nodes |
| DecisionTree depth=2, leaf=2 | 0.775428 | 0.774510 | 5 nodes |
| DecisionTree depth=2, leaf=3 | 0.775428 | 0.774510 | 5 nodes |
| DecisionTree depth=2, leaf=5 | 0.775428 | 0.774510 | 5 nodes |
| DecisionTree depth=3, leaf=1 | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=3, leaf=2 | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=3, leaf=3 | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=3, leaf=5 | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=4, leaf=1 | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=4, leaf=2 | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=4, leaf=3 | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=4, leaf=5 | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=5, leaf=1 | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=5, leaf=2 | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=5, leaf=3 | 0.775428 | 0.774510 | 7 nodes |
| DecisionTree depth=5, leaf=5 | 0.775428 | 0.774510 | 7 nodes |
| LogisticRegression C=0.01 | 0.786325 | 0.794118 | 33 coefficients |
| LogisticRegression C=0.03 | 0.912323 | 0.911765 | 33 coefficients |
| LogisticRegression C=0.1 | 0.856523 | 0.852941 | 33 coefficients |
| LogisticRegression C=0.3 | 0.783104 | 0.784314 | 33 coefficients |
| LogisticRegression C=1 | 0.740334 | 0.735294 | 33 coefficients |
| RandomForest n_estimators=10, depth=3 | 0.749550 | 0.745098 | 80 total nodes |
| RandomForest n_estimators=10, depth=4 | 0.749550 | 0.745098 | 90 total nodes |
| RandomForest n_estimators=20, depth=3 | 0.820873 | 0.813725 | 166 total nodes |
| RandomForest n_estimators=20, depth=4 | 0.830203 | 0.823529 | 198 total nodes |

## Relative vs absolute features (same subset, diagnostic only)

These baseline-subset comparisons are diagnostic only and are excluded from model selection.

| Candidate | Feature set | Macro-F1 | Accuracy | Model size |
|---|---|---:|---:|---:|
| DecisionTree depth=2, leaf=1 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=2, leaf=2 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=2, leaf=3 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=2, leaf=5 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=3, leaf=1 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=3, leaf=2 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=3, leaf=3 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=3, leaf=5 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=4, leaf=1 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=4, leaf=2 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=4, leaf=3 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=4, leaf=5 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=5, leaf=1 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=5, leaf=2 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=5, leaf=3 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| DecisionTree depth=5, leaf=5 | absolute on baseline subset | 0.903922 | 0.928571 | 5 nodes |
| LogisticRegression C=0.01 | absolute on baseline subset | 0.557078 | 0.726190 | 33 coefficients |
| LogisticRegression C=0.03 | absolute on baseline subset | 0.721492 | 0.785714 | 33 coefficients |
| LogisticRegression C=0.1 | absolute on baseline subset | 0.802873 | 0.833333 | 33 coefficients |
| LogisticRegression C=0.3 | absolute on baseline subset | 0.802873 | 0.833333 | 33 coefficients |
| LogisticRegression C=1 | absolute on baseline subset | 0.802873 | 0.833333 | 33 coefficients |
| RandomForest n_estimators=10, depth=3 | absolute on baseline subset | 0.827002 | 0.857143 | 68 total nodes |
| RandomForest n_estimators=10, depth=4 | absolute on baseline subset | 0.862418 | 0.880952 | 82 total nodes |
| RandomForest n_estimators=20, depth=3 | absolute on baseline subset | 0.851753 | 0.880952 | 138 total nodes |
| RandomForest n_estimators=20, depth=4 | absolute on baseline subset | 0.827002 | 0.857143 | 170 total nodes |
| DecisionTree depth=2, leaf=1 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=2, leaf=2 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=2, leaf=3 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=2, leaf=5 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=3, leaf=1 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=3, leaf=2 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=3, leaf=3 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=3, leaf=5 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=4, leaf=1 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=4, leaf=2 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=4, leaf=3 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=4, leaf=5 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=5, leaf=1 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=5, leaf=2 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=5, leaf=3 | relative | 0.889905 | 0.916667 | 5 nodes |
| DecisionTree depth=5, leaf=5 | relative | 0.889905 | 0.916667 | 5 nodes |
| LogisticRegression C=0.01 | relative | 0.574253 | 0.750000 | 33 coefficients |
| LogisticRegression C=0.03 | relative | 0.546622 | 0.690476 | 33 coefficients |
| LogisticRegression C=0.1 | relative | 0.763535 | 0.809524 | 33 coefficients |
| LogisticRegression C=0.3 | relative | 0.815874 | 0.845238 | 33 coefficients |
| LogisticRegression C=1 | relative | 0.815874 | 0.845238 | 33 coefficients |
| RandomForest n_estimators=10, depth=3 | relative | 0.864461 | 0.892857 | 70 total nodes |
| RandomForest n_estimators=10, depth=4 | relative | 0.827002 | 0.857143 | 86 total nodes |
| RandomForest n_estimators=20, depth=3 | relative | 0.864461 | 0.892857 | 140 total nodes |
| RandomForest n_estimators=20, depth=4 | relative | 0.864461 | 0.892857 | 174 total nodes |

Best learned candidate: LogisticRegression C=0.03 (macro-F1=0.912323, accuracy=0.911765); selected on full training data only.

## Per-session leave-one-session-out results

| Session | Label | Windows | Rule correct | Best-tree correct | Selected-model correct |
|---|---:|---:|---:|---:|---:|
| train_s0_1 | 0 | 6 | 6 | 6 | 6 |
| train_s0_2 | 0 | 6 | 6 | 6 | 6 |
| train_s0_3 | 0 | 6 | 0 | 0 | 6 |
| train_s0_4 | 0 | 6 | 6 | 6 | 6 |
| train_s0_5 | 0 | 6 | 6 | 6 | 6 |
| train_s0_6 | 0 | 6 | 5 | 0 | 6 |
| train_s1_1 | 1 | 6 | 6 | 1 | 6 |
| train_s1_2 | 1 | 5 | 2 | 5 | 5 |
| train_s1_3 | 1 | 6 | 6 | 0 | 6 |
| train_s1_4 | 1 | 6 | 6 | 6 | 6 |
| train_s1_6 | 1 | 12 | 9 | 12 | 9 |
| train_s2_1 | 2 | 6 | 6 | 6 | 0 |
| train_s2_2 | 2 | 5 | 5 | 5 | 5 |
| train_s2_5 | 2 | 6 | 6 | 6 | 6 |
| train_s2_6 | 2 | 14 | 14 | 14 | 14 |

## Pooled LOSO confusion matrix: Rule

Rows=true, columns=predicted (states 0, 1, 2).

| True \ Predicted | 0 | 1 | 2 |
|---:|---:|---:|---:|
| 0 | 29 | 7 | 0 |
| 1 | 3 | 29 | 3 |
| 2 | 0 | 0 | 31 |

| State | Precision | Recall |
|---:|---:|---:|
| 0 | 0.906250 | 0.805556 |
| 1 | 0.805556 | 0.828571 |
| 2 | 0.911765 | 1.000000 |

## Pooled LOSO confusion matrix: Selected model

Rows=true, columns=predicted (states 0, 1, 2).

| True \ Predicted | 0 | 1 | 2 |
|---:|---:|---:|---:|
| 0 | 36 | 0 | 0 |
| 1 | 3 | 32 | 0 |
| 2 | 6 | 0 | 25 |

| State | Precision | Recall |
|---:|---:|---:|
| 0 | 0.800000 | 1.000000 |
| 1 | 1.000000 | 0.914286 |
| 2 | 1.000000 | 0.806452 |

## Chosen tree fitted on all training data

```text
|--- amb_std <= 1.50
|   |--- rh_mean <= 85.26
|   |   |--- class: 0
|   |--- rh_mean >  85.26
|   |   |--- class: 2
|--- amb_std >  1.50
|   |--- class: 1
```

<!-- cv_metadata: {"chosen": {"max_depth": 2, "min_samples_leaf": 1}} -->
