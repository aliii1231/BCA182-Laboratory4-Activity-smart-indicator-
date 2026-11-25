# Held-out test results

Test window count: 36

## Rule

Confusion matrix (rows=true, columns=predicted; states 0, 1, 2):

| True \ Predicted | 0 | 1 | 2 |
|---:|---:|---:|---:|
| 0 | 6 | 0 | 6 |
| 1 | 3 | 3 | 6 |
| 2 | 0 | 0 | 12 |

| State | Precision | Recall | F1 |
|---:|---:|---:|---:|
| 0 | 0.666667 | 0.500000 | 0.571429 |
| 1 | 1.000000 | 0.250000 | 0.400000 |
| 2 | 0.500000 | 1.000000 | 0.666667 |

Overall accuracy: 0.583333

Misclassified windows:

| session_id | window | true label | predicted label |
|---|---:|---:|---:|
| test_s0_2 | 1 | 0 | 2 |
| test_s0_2 | 2 | 0 | 2 |
| test_s0_2 | 3 | 0 | 2 |
| test_s0_2 | 4 | 0 | 2 |
| test_s0_2 | 5 | 0 | 2 |
| test_s0_2 | 6 | 0 | 2 |
| test_s1_1 | 2 | 1 | 0 |
| test_s1_1 | 4 | 1 | 0 |
| test_s1_1 | 6 | 1 | 0 |
| test_s1_2 | 1 | 1 | 2 |
| test_s1_2 | 2 | 1 | 2 |
| test_s1_2 | 3 | 1 | 2 |
| test_s1_2 | 4 | 1 | 2 |
| test_s1_2 | 5 | 1 | 2 |
| test_s1_2 | 6 | 1 | 2 |

## Tree

Confusion matrix (rows=true, columns=predicted; states 0, 1, 2):

| True \ Predicted | 0 | 1 | 2 |
|---:|---:|---:|---:|
| 0 | 12 | 0 | 0 |
| 1 | 0 | 12 | 0 |
| 2 | 0 | 0 | 12 |

| State | Precision | Recall | F1 |
|---:|---:|---:|---:|
| 0 | 1.000000 | 1.000000 | 1.000000 |
| 1 | 1.000000 | 1.000000 | 1.000000 |
| 2 | 1.000000 | 1.000000 | 1.000000 |

Overall accuracy: 1.000000

Misclassified windows:

None.

## Selected model

Confusion matrix (rows=true, columns=predicted; states 0, 1, 2):

| True \ Predicted | 0 | 1 | 2 |
|---:|---:|---:|---:|
| 0 | 12 | 0 | 0 |
| 1 | 6 | 6 | 0 |
| 2 | 0 | 0 | 12 |

| State | Precision | Recall | F1 |
|---:|---:|---:|---:|
| 0 | 0.666667 | 1.000000 | 0.800000 |
| 1 | 1.000000 | 0.500000 | 0.666667 |
| 2 | 1.000000 | 1.000000 | 1.000000 |

Overall accuracy: 0.833333

Misclassified windows:

| session_id | window | true label | predicted label |
|---|---:|---:|---:|
| test_s1_1 | 2 | 1 | 0 |
| test_s1_1 | 4 | 1 | 0 |
| test_s1_1 | 6 | 1 | 0 |
| test_s1_2 | 2 | 1 | 0 |
| test_s1_2 | 4 | 1 | 0 |
| test_s1_2 | 6 | 1 | 0 |
