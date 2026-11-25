#!/usr/bin/env python3
"""Export the frozen logistic model and held-out port-check vectors as C headers."""
import json
from pathlib import Path

import joblib
import numpy as np
import pandas as pd
import sklearn
from sklearn.linear_model import LogisticRegression
from sklearn.preprocessing import StandardScaler

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "ml" / "out"
INCLUDE = ROOT / "include"
PARAMS_PATH = INCLUDE / "model_params.h"
VECTORS_PATH = INCLUDE / "model_selftest_vectors.h"
REPORT_PATH = OUT / "export_report.md"


def c_float(value):
    return f"{float(value):.9g}f"


def array_1d(name, values):
    entries = ", ".join(c_float(value) for value in values)
    return f"static const float {name}[{len(values)}] = {{{entries}}};"


def main():
    frozen = json.loads((OUT / "frozen.json").read_text(encoding="utf-8"))
    features = frozen["features"]
    if len(features) != 11:
        raise ValueError(f"expected 11 frozen features, got {len(features)}")

    saved = joblib.load(OUT / "model.joblib")
    model = saved["selected_model"]
    if len(model) < 2 or not isinstance(model[0], StandardScaler) or not isinstance(model[-1], LogisticRegression):
        raise TypeError("saved selected_model must be a StandardScaler + LogisticRegression pipeline")
    scaler, classifier = model[0], model[-1]
    if list(saved["features"]) != features:
        raise ValueError("frozen feature order does not match saved model feature order")
    if list(classifier.classes_) != [0, 1, 2]:
        raise ValueError(f"expected model classes [0, 1, 2], got {classifier.classes_}")
    if scaler.mean_.shape != (11,) or scaler.scale_.shape != (11,) or classifier.coef_.shape != (3, 11):
        raise ValueError("saved scaler or coefficient dimensions do not match the 11-feature model")

    family = frozen["selected_model"]["family"]
    c_value = frozen["selected_model"]["hyperparameters"]["C"]
    params_header = [
        "/*",
        f" * Model family: {family}",
        f" * C: {c_value}",
        f" * sklearn version: {frozen['sklearn_version']}",
        f" * Git commit: {frozen['git_commit']}",
        f" * Timestamp UTC: {frozen['timestamp_utc']}",
        " * GENERATED - do not edit",
        " * Feature order: " + ", ".join(features),
        " * Window statistics use population std (ddof=0); delta = last minus first.",
        " */",
        "#ifndef MODEL_PARAMS_H",
        "#define MODEL_PARAMS_H",
        "",
        "#define MODEL_N_FEATURES 11",
        "#define MODEL_N_CLASSES 3",
        "#define MODEL_WINDOW_SAMPLES 10",
        "",
        array_1d("model_mean", scaler.mean_),
        array_1d("model_scale", scaler.scale_),
        "static const float model_coef[3][11] = {",
    ]
    params_header += ["    {" + ", ".join(c_float(v) for v in row) + "},"
                      for row in classifier.coef_]
    params_header += ["};", array_1d("model_intercept", classifier.intercept_),
                      "", "#endif /* MODEL_PARAMS_H */", ""]
    PARAMS_PATH.write_text("\n".join(params_header), encoding="utf-8")

    test = pd.read_csv(OUT / "test_windows.csv")
    if len(test) != 36:
        raise ValueError(f"expected 36 test windows for self-test vectors, got {len(test)}")
    x = test[features].to_numpy(dtype=np.float64)
    expected = model.predict(x).astype(np.uint8)
    labels = test["label"].to_numpy(dtype=np.uint8)
    vector_header = [
        "/* GENERATED - do not edit; float64 Python predictions from frozen model. */",
        "#ifndef MODEL_SELFTEST_VECTORS_H",
        "#define MODEL_SELFTEST_VECTORS_H",
        "",
        "#include <stdint.h>",
        "",
        "static const float selftest_features[36][11] = {",
    ]
    vector_header += ["    {" + ", ".join(c_float(v) for v in row) + "}," for row in x]
    vector_header += ["};", "", "static const uint8_t selftest_expected[36] = {",
                      "    " + ", ".join(str(int(v)) for v in expected), "};", "",
                      "static const uint8_t selftest_label[36] = {",
                      "    " + ", ".join(str(int(v)) for v in labels), "};", "",
                      "#endif /* MODEL_SELFTEST_VECTORS_H */", ""]
    VECTORS_PATH.write_text("\n".join(vector_header), encoding="utf-8")

    parameter_count = int(scaler.mean_.size + scaler.scale_.size +
                          classifier.coef_.size + classifier.intercept_.size)
    float_count = parameter_count + int(x.size)
    byte_count = float_count * 4 + int(expected.size + labels.size)
    report = ("# C model export report\n\n"
              f"Model parameter count: {parameter_count}\n\n"
              f"Const array payload: {byte_count} bytes "
              f"({float_count} float32 values and {expected.size + labels.size} uint8 values).\n\n"
              f"Self-test vectors: {len(test)}; predictions generated with sklearn {sklearn.__version__}.\n\n"
              "This is a port verification, not a second evaluation.\n")
    REPORT_PATH.write_text(report, encoding="utf-8")
    print(report, end="")


if __name__ == "__main__":
    main()
