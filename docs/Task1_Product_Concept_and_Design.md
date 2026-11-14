# Task 1: Product Concept and Design - Smart Storage Indicator

BCA182 Laboratory Activity 4 | RT-Thread RT-Spark (STM32F407ZGT6) | Draft v0.1

## 1. Product concept

The Smart Storage Indicator is a small indicator that sits inside a cabinet, drawer, pantry box or storage container used to keep sensitive household items: non-refrigerated medicines, tea and coffee, spices, dry foods, cosmetics, documents or photographs. The target user is a consumer who stores such items but cannot tell whether the enclosure was opened and left open, or exposed to excess heat or humidity, until the item is already spoiled. The product measures temperature and relative humidity (AHT21) and ambient light and proximity (AP3216C), classifies the storage condition on the STM32F407 itself, and tells the user locally (LCD, LED matrix, LEDs, buzzer) whether the contents are safe, being accessed, or at spoilage risk. The main goal is to warn the user of spoilage risk early, without any Internet connection.

The product is an experimental prototype. It is not a certified food-safety, pharmaceutical-storage or medical device, and its output must not be relied on for safety-critical decisions.

## 2. Alternative ideas considered

| Idea | Summary | Why it was not selected |
|---|---|---|
| Context-Aware Study Desk | Uses ambient light, proximity and temperature to judge whether a desk is suitable for studying and whether the user is present. | Ground-truth labels are subjective (what counts as "good" lighting), the state differences are weak, and it closely resembles common tutorial projects. |
| Household Object-Handling Monitor | Uses the ICM-20608 IMU to classify handling of a stored item (still, carried, shaken, dropped). | Needs a safe and repeatable protocol for drops and impacts, does not use the environmental sensors in the primary function, and makes the AHT21/AP3216C hardware largely idle. |
| **Smart Storage Indicator (selected)** | Classifies storage condition from environmental sensing. | Has a clear user and a measurable input, three states that can be reproduced safely with household materials, and uses two distinct sensing modalities (thermal/humidity and light/proximity) on the board. |

## 3. Classification states

| Label | State | Definition (provisional) | Local output |
|---|---|---|---|
| 0 | Sealed and Safe Storage | Enclosure closed and dark; temperature and humidity within the room's normal range. | Steady low-brightness green on the LED matrix, "SAFE" on the LCD. |
| 1 | Active Access / Door Open | Sudden light exposure and/or a close proximity reading; temperature near ambient. | Blue pulse, "OPEN" on the LCD, no buzzer unless left open too long. |
| 2 | Spoilage Risk / Adverse Exposure | High temperature or high humidity, with or without light. | Red indicator, "RISK" on the LCD, buzzer pattern until acknowledged. |

State 2 must be recorded both in the dark and with light, so that the model does not learn "dark means not risky".

## 4. System block diagram

```
 AHT21 (soft I2C)  --\
                       \                                        LCD (ST7789, FSMC)
 AP3216C (I2C2)    ----->  Sensor task --> queue --> Inference ---> LED matrix / LEDs
                       /     (fixed rate,  task      task            Buzzer (PWM)
 Buttons (GPIO)    --/       validity)       |           |
                                             v           v
                                      Logger task   UI/diagnostic task
                                      (serial, RTC     ^
                                       timestamps,     |
                                       SD/Flash)    Buttons: acknowledge,
                                                    mute, diagnostics, self-test
 Diagnostic/service: ICM-20608, RTC, SD, SPI Flash, Wi-Fi (RW007), IR TX/RX
```

All inference and the immediate user response run on the STM32F407. Wi-Fi, where used, only carries event records.

## 5. Component-to-function matrix

Status values are filled in during Task 2 bring-up on the actual kit.

| Component | Role | Function in the product | Verified on kit |
|---|---|---|---|
| AHT21 (temperature/humidity, soft I2C, addr 0x38) | Primary sensing | Temperature and RH inputs to the classifier. | Live readings confirmed |
| AP3216C (ambient light/proximity, I2C2, addr 0x1E) | Primary sensing | Light exposure and hand proximity inputs to the classifier. | Live readings confirmed |
| ST7789 LCD (FSMC) | Local output | Shows state, readings and warnings. | Pending |
| LED matrix (19 x SK6805) | Local output | Shows the state with color and pattern. | Pending |
| Red/blue LEDs | Local output | Status and fault indicators. | Pending |
| Buzzer | Local output | Warning tone for state 2 and for left-open events. | Pending |
| Buttons (up/down/left/right) | User input | Acknowledge, mute, change diagnostic page, self-test. | Pending (events print on serial) |
| ICM-20608 IMU (I2C2, addr 0x68) | Diagnostic/service | Detects that the enclosure was moved or tilted, sensor health check. | WHO_AM_I 0xAF confirmed; accel/gyro pending |
| RTC | Diagnostic/service | Timestamps for logs. | Pending |
| microSD, SPI Flash (W25Q128) | Diagnostic/service | Local event log and configuration storage. | Pending |
| Wi-Fi (RW007) | Diagnostic/service | Optional event reporting; never required for inference. | Pending |
| IR transmitter/receiver | Diagnostic/service | Self-test only. | Pending |
| CAN, RS485, USB, expansion headers | Not used | No attached module; documented as unused. | n/a |

## 6. Acceptance criteria

These are targets. They will be confirmed or revised after the dataset and bring-up measurements exist.

| Area | Target |
|---|---|
| Held-out accuracy | Per-class recall at least 90 percent for State 2 and at least 85 percent for States 0 and 1, on windows from sessions not used for training. |
| Errors that matter | Missed State 2 (spoilage risk reported as safe) is the most serious error. False State 2 alarms in States 0 and 1 should be at most 5 percent. |
| Baseline | The trained model must beat the threshold baseline in section 8 on the same held-out windows, or the report states that it did not. |
| Response time | State change shown on the LCD and LED matrix within one window length plus 2 s after the condition begins. |
| Inference | At least 50 on-board runs; median and 95th percentile time reported. |
| Offline | At least 5 consecutive minutes of correct operation with no network. |
| Soak | 15 minute continuous run with no crash, hang or unresponsive button. |
| Memory | Model Flash and RAM footprint and total firmware use reported; task stack high-water marks above zero margin. |

Measurement plan: 30 training and 10 held-out windows per class from separate sessions, then 20 further independent trials per state that are not used for training or tuning, using the same procedure each time (see the capture protocol).

## 7. Risks

- **Privacy:** The device records only environmental values and timestamps, not images, audio or personal data. Optional Wi-Fi reports must not include personal identifiers, and no credentials are committed to the repository.
- **Accessibility:** State is never conveyed by color alone: the LCD shows text, the LED matrix uses distinct patterns, and the buzzer pattern differs per state. A mute button lets users silence the buzzer, and a visible indicator remains.
- **Safety:** State 2 is simulated with warm (not hot) water and exhaled breath only, with no liquid contact with the board and no flames or heat guns. The board should not be allowed to exceed about 45 degrees Celsius during tests. False reassurance is the main user risk, so the product is labeled an experimental prototype and does not claim to protect any item.

## 8. Baseline (threshold) rule

Compared against the trained model on the same held-out windows. Values are provisional and are set from the collected dataset, not from guesses.

```
if temp_c >= T_HI or rh_pct >= RH_HI:      state = 2
elif ambient_raw >= L_OPEN or proximity_raw >= P_NEAR:  state = 1
else:                                       state = 0
```

Starting values for tuning: T_HI about 38 C, RH_HI about 85 percent, L_OPEN about 50 raw counts above the measured dark baseline, P_NEAR taken from the proximity values seen when a hand is near the sensor.

## 9. Known limitations and open items

- The first bring-up reading was 32.96 C and 72.93 percent RH with `ambient_raw=10` and `proximity_raw=0`. These are uncalibrated and the temperature is probably elevated by board self-heating. A calibration offset (`cal_temp_c`) will be determined and documented.
- The LCD backlight and LED matrix can leak light into the AP3216C. The effect will be measured and, if significant, the outputs will be dimmed or timed so they do not corrupt the light reading.
- The sensor interval is currently 1 s. A short lid opening produces only a few samples at this rate, so the sampling rate must be fixed before data collection.
- The labels depend on how the states are simulated. The simulation procedure is part of the dataset documentation.