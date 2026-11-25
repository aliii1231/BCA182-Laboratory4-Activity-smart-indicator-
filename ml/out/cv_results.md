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
| LogisticRegression C=0.1 | absolute | 0.856523 | 0.852941 | 33 coefficients |
| LogisticRegression C=1 | absolute | 0.740334 | 0.735294 | 33 coefficients |
| LogisticRegression C=10 | absolute | 0.740334 | 0.735294 | 33 coefficients |
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
| LogisticRegression C=0.1 | absolute on baseline subset | 0.802873 | 0.833333 | 33 coefficients |
| LogisticRegression C=1 | absolute on baseline subset | 0.802873 | 0.833333 | 33 coefficients |
| LogisticRegression C=10 | absolute on baseline subset | 0.779085 | 0.809524 | 33 coefficients |
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
| LogisticRegression C=0.1 | relative | 0.763535 | 0.809524 | 33 coefficients |
| LogisticRegression C=1 | relative | 0.815874 | 0.845238 | 33 coefficients |
| LogisticRegression C=10 | relative | 0.782825 | 0.809524 | 33 coefficients |
| RandomForest n_estimators=10, depth=3 | relative | 0.864461 | 0.892857 | 70 total nodes |
| RandomForest n_estimators=10, depth=4 | relative | 0.827002 | 0.857143 | 86 total nodes |
| RandomForest n_estimators=20, depth=3 | relative | 0.864461 | 0.892857 | 140 total nodes |
| RandomForest n_estimators=20, depth=4 | relative | 0.864461 | 0.892857 | 174 total nodes |

Best candidate: DecisionTree depth=2, leaf=1 using absolute on baseline subset features, macro-F1=0.903922, accuracy=0.928571. Chosen on training data only.

## Per-session leave-one-session-out results

| Session | Label | Windows | Rule correct | Best tree correct |
|---|---:|---:|---:|---:|
| train_s0_1 | 0 | 6 | 6 | 6 |
| train_s0_2 | 0 | 6 | 6 | 6 |
| train_s0_3 | 0 | 6 | 0 | 0 |
| train_s0_4 | 0 | 6 | 6 | 6 |
| train_s0_5 | 0 | 6 | 6 | 6 |
| train_s0_6 | 0 | 6 | 5 | 0 |
| train_s1_1 | 1 | 6 | 6 | 1 |
| train_s1_2 | 1 | 5 | 2 | 5 |
| train_s1_3 | 1 | 6 | 6 | 0 |
| train_s1_4 | 1 | 6 | 6 | 6 |
| train_s1_6 | 1 | 12 | 9 | 12 |
| train_s2_1 | 2 | 6 | 6 | 6 |
| train_s2_2 | 2 | 5 | 5 | 5 |
| train_s2_5 | 2 | 6 | 6 | 6 |
| train_s2_6 | 2 | 14 | 14 | 14 |

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
