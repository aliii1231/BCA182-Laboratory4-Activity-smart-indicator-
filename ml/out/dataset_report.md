# Dataset build report

## Per session

train_s0_1: windows built=6, windows excluded=0
train_s0_2: windows built=6, windows excluded=0
train_s0_3: windows built=6, windows excluded=0
train_s0_4: windows built=6, windows excluded=0
train_s0_5: windows built=6, windows excluded=0
train_s0_6: windows built=6, windows excluded=0
train_s1_1: windows built=6, windows excluded=0
train_s1_2: windows built=5, windows excluded=1
train_s1_3: windows built=6, windows excluded=0
train_s1_4: windows built=6, windows excluded=0
train_s1_6: windows built=12, windows excluded=0
train_s2_1: windows built=6, windows excluded=0
train_s2_2: windows built=5, windows excluded=1
train_s2_3: windows built=0, windows excluded=6
train_s2_4: windows built=0, windows excluded=6
train_s2_5: windows built=6, windows excluded=0
train_s2_6: windows built=14, windows excluded=0
test_s0_1: windows built=6, windows excluded=0
test_s0_2: windows built=6, windows excluded=0
test_s1_1: windows built=6, windows excluded=0
test_s1_2: windows built=6, windows excluded=0
test_s2_1: windows built=6, windows excluded=0
test_s2_2: windows built=6, windows excluded=0

## Excluded sessions and windows

train_s1_2 w2: no lid opening or hand in window
train_s2_2 w1: humidity condition not yet reached (+1.2 %)
train_s2_3 all: warm bottle near sensor: proximity 110-120 in all windows (normal 8-38)
train_s2_4 all: humidity 80.5-80.9 % is below the fresh State 0 baseline; risk condition not established

## Windows per split and state

| Split | State | Windows |
|---|---:|---:|
| train | 0 | 36 |
| train | 1 | 35 |
| train | 2 | 31 |
| test | 0 | 12 |
| test | 1 | 12 |
| test | 2 | 12 |
