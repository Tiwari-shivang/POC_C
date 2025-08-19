# Laptop-Only Embedded C PoC — Feature Spec (for Claude)

**Goal:** Build a *single* C application that simulates multiple in-vehicle features on a laptop (no MCU), using an embedded-firmware structure (HAL + app modules), MISRA-minded coding style, and reproducible scenarios.

---

## Runtime Model (No Hardware)
- **Scheduler:** 10 ms tick loop calling each feature’s `*_step()`.
- **HAL:** Abstract interfaces. For PoC we implement **PC mocks** reading inputs from a **scenario CSV** and logging outputs to a CSV.
- **Modules:** Each feature lives in a separate `app_*.c` with **pure logic** (no I/O).
- **Testing:** Unity-based unit tests per module.
- **Standards mindset:** C99, fixed-width types, no dynamic allocation, bounded loops/counters, deterministic flow, static analysis.

---

## Dependencies (PC only)
- **Build:** GCC/Clang + CMake (>= 3.16)
- **Static analysis:** Cppcheck, clang-tidy (CERT profile)
- **Unit tests:** Unity (+ optional CMock/Ceedling)
- **Formatting:** clang-format
- *(Optional)* visualization: SDL2 (for a small UI), or plot CSV in any tool

**Compiler flags (suggested):**
```
-std=c99 -O0 -g3 -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wformat=2 -Wundef
```

**Static analysis (suggested):**
```
cppcheck --enable=warning,performance,portability --std=c99 --inline-suppr src inc
clang-tidy src/*.c -- -std=c99
```

---

## Project Structure
```
car_poc/
├─ CMakeLists.txt
├─ README.md
├─ docs/
│  ├─ requirements.md         # brief FR/SR list + hazards
│  ├─ misra_policy.md         # rules followed + deviations log
│  └─ test_plan.md
├─ cfg/
│  ├─ calib.h                 # thresholds, hysteresis, controller gains
│  └─ scenario_default.csv    # time-series of inputs/events
├─ inc/
│  ├─ platform.h              # timebase, assert, compile-time config
│  ├─ hal.h                   # abstract HW interfaces (sensor/actuator)
│  ├─ app_autobrake.h
│  ├─ app_wipers.h
│  ├─ app_speedgov.h
│  ├─ app_autopark.h
│  ├─ app_climate.h
│  └─ app_voice.h
├─ src/
│  ├─ main.c                  # 10 ms scheduler tick
│  ├─ platform_pc.c           # timing/log helpers
│  ├─ hal_mock_pc.c           # scenario reader + output logger
│  ├─ io_logger.c             # CSV logger for outputs
│  ├─ app_autobrake.c         # pure logic
│  ├─ app_wipers.c
│  ├─ app_speedgov.c
│  ├─ app_autopark.c
│  ├─ app_climate.c
│  └─ app_voice.c
├─ sim/
│  ├─ scenario.h              # CSV schema + parser
│  ├─ scenario.c
│  └─ scenarios/              # sample drives
│     ├─ city_50kph.csv
│     ├─ highway_100kph.csv
│     └─ rain_parking.csv
├─ tests/
│  ├─ unity/
│  ├─ test_autobrake.c
│  ├─ test_wipers.c
│  ├─ test_speedgov.c
│  ├─ test_autopark.c
│  └─ test_climate.c
└─ tools/
   ├─ run_static.sh           # cppcheck + clang-tidy
   └─ format.sh               # clang-format
```

---

## Common Interfaces

### `inc/hal.h` (PC mock implements these)
```c
#ifndef HAL_H
#define HAL_H
#include <stdint.h>
#include <stdbool.h>

bool     hal_get_vehicle_ready(void);
bool     hal_driver_brake_pressed(void);
uint32_t hal_now_ms(void);

/* Sensors (simulated via scenario CSV) */
bool hal_read_distance_mm(uint16_t* out_mm, uint32_t* out_ts_ms);
bool hal_read_rain_level_pct(uint8_t* out_pct, uint32_t* out_ts_ms);
bool hal_read_vehicle_speed_kph(uint16_t* out_kph, uint32_t* out_ts_ms);
bool hal_poll_speed_limit_kph(uint16_t* out_limit_kph);  /* event: sign seen */
bool hal_read_cabin_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms);
bool hal_read_ambient_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms);
bool hal_read_humidity_pct(uint8_t* out_pct, uint32_t* out_ts_ms);
bool hal_read_voice_line(char* buf, uint16_t len);       /* non-blocking */

/* Actuators (captured by logger) */
void hal_set_brake_request(bool on);
void hal_set_wiper_mode(uint8_t mode);   /* 0=OFF,1=INT,2=LOW,3=HIGH */
void hal_set_alarm(bool on);
void hal_set_speed_limit_request(uint16_t kph);
void hal_set_climate(uint8_t fan_stage, bool ac_on, uint8_t blend_pct);
void hal_actuate_parking_prompt(uint8_t step_code);
#endif
```

### `cfg/calib.h` (tunable constants; keep integers)
```c
#define TICK_MS                    (10U)
#define SENSOR_STALE_MS            (100U)

/* Auto Braking */
#define AB_THRESHOLD_MM            (1220U)   /* ≈ 4 ft */
#define AB_DEBOUNCE_HITS          (3U)

/* Wipers */
#define WIPER_T_RAIN_INT          (20U)     /* % */
#define WIPER_T_RAIN_LOW          (45U)
#define WIPER_T_RAIN_HIGH         (75U)

/* Speed Governor */
#define SPEED_ALARM_TOL_KPH       (3U)
#define SPEED_ALARM_DEBOUNCE      (2U)
#define SPEED_HYSTERESIS_KPH      (5U)

/* Climate (discrete PI; integer math) */
#define CLIMATE_KP                (8)
#define CLIMATE_KI                (1)
#define CLIMATE_DT_MS             (1000U)

/* Auto Parking (very simplified) */
#define PARK_MIN_GAP_MM           (5000U)
#define PARK_SCAN_LEN_SAMPLES     (50U)
```

---

## Scenario CSV (example columns)
`ms, distance_mm, rain_pct, speed_kph, sign_event, cabin_tc_x10, ambient_tc_x10, humid_pct, user_setpoint_x10, voice_cmd`
- **ms:** monotonic time (row timebase)
- **sign_event:** 0 if none, else speed limit (30/50/80/100…)
- **voice_cmd:** optional string (`hey car open sunroof`)

---

## Build & Run
```bash
mkdir build && cd build
cmake ..
make -j
./car_poc --scenario ./cfg/scenario_default.csv
# outputs logged to ./outputs.csv for plotting
```
Run `tools/run_static.sh` for static analysis; run unit tests with `ctest` or a Ceedling task if used.

---

## MISRA/CERT Guardrails (applies to all modules)
- C99; **fixed-width types**; **no dynamic memory** or recursion; **bounded** loops/counters.
- Check and handle **all return values**; initialize all variables; explicit casts only when needed.
- Keep feature modules **pure** (no I/O); isolate hardware/OS in `hal_*`/`platform_*`.
- Automate static analysis; keep a **deviations log** in `docs/misra_policy.md`.

---

# Features (Working + Build Notes)

## 1) Auto Braking
**Purpose:** Request braking if an obstacle is within a calibrated distance.
- **Inputs:** `distance_mm` (+ timestamp), vehicle ready, driver brake override.
- **Outputs:** `brake_request` (bool).
- **Logic (brief):**
  - Validate freshness (`now - ts <= SENSOR_STALE_MS`).
  - If valid & not inhibited (vehicle not ready / driver pressed brake), debounce **N** consecutive samples `<= AB_THRESHOLD_MM` → `brake_request = true`.
  - Else clear hits and command `false`.
- **Build steps:**
  1. Create `inc/app_autobrake.h` with `void app_autobrake_step(void);`
  2. Implement `src/app_autobrake.c` (pure logic; calls `hal_*` only).
  3. Add unit tests in `tests/test_autobrake.c` (debounce, stale, override).
  4. Call from scheduler in `src/main.c` every 10 ms.
- **Key calib:** `AB_THRESHOLD_MM`, `AB_DEBOUNCE_HITS`.

## 2) Rain-Sensing Wipers
**Purpose:** Map rain intensity to wiper mode.
- **Inputs:** `rain_pct`, `speed_kph` (optional for higher speed bias).
- **Outputs:** `wiper_mode` = OFF/INT/LOW/HIGH.
- **Logic:**
  - Hysteresis around thresholds: INT, LOW, HIGH (use previous mode to avoid chatter).
  - Optional: increase mode one level if `speed_kph` high.
- **Build steps:**
  1. `inc/app_wipers.h` → `void app_wipers_step(void);`
  2. `src/app_wipers.c` → threshold + hysteresis state machine.
  3. Tests: mode transitions up/down; noise around thresholds.
- **Key calib:** `WIPER_T_RAIN_INT/LOW/HIGH`.

## 3) Speed Governor + Traffic Sign Reading
**Purpose:** Raise alarm (and optional limit request) when vehicle exceeds current speed limit.
- **Inputs:** `speed_kph`, `sign_event` (updates `current_limit`).
- **Outputs:** `alarm_on`, `limit_request_kph` (optional).
- **Logic:**
  - Update `current_limit` when a sign event arrives.
  - Alarm if `speed_kph > current_limit + SPEED_ALARM_TOL_KPH` for `SPEED_ALARM_DEBOUNCE` ticks.
  - Clear when `speed_kph < current_limit - SPEED_HYSTERESIS_KPH`.
- **Build steps:**
  1. `inc/app_speedgov.h` → `void app_speedgov_step(void);`
  2. `src/app_speedgov.c` → debounce + hysteresis.
  3. Tests: over-speed latch/clear; sign changes at runtime.
- **Key calib:** `SPEED_*` macros in `calib.h`.

## 4) Auto Parking (Simplified Guidance)
**Purpose:** Detect a gap and emit simple parking prompts.
- **Inputs:** Side “distance scan” stream (from scenario) and low vehicle speed.
- **Outputs:** Step prompts via `hal_actuate_parking_prompt(step_code)`.
- **Logic:**
  - Scan window of `PARK_SCAN_LEN_SAMPLES`; detect a continuous region `>= PARK_MIN_GAP_MM`.
  - Once detected, enter guidance sequence: e.g., `STEP_FIND → STEP_REVERSE_RIGHT → STEP_STRAIGHTEN → STEP_REVERSE_LEFT → STEP_DONE` with fixed durations.
  - Abort on timeout/speed too high.
- **Build steps:**
  1. `inc/app_autopark.h` → `void app_autopark_step(void);`
  2. `src/app_autopark.c` → simple gap-detect + scripted steps.
  3. Tests: gap detection; sequence timing; abort conditions.
- **Key calib:** `PARK_MIN_GAP_MM`, `PARK_SCAN_LEN_SAMPLES`.

## 5) Auto Climate Control
**Purpose:** Maintain cabin temperature near user setpoint with simple discrete PI and staged fan.
- **Inputs:** `cabin_temp_x10`, `ambient_temp_x10`, `humidity_pct`, `user_setpoint_x10`.
- **Outputs:** `ac_on` (bool), `fan_stage` (0–3), `blend_pct` (0–100).
- **Logic:**
  - Error = setpoint − cabin; PI accumulator with `CLIMATE_KP/KI`, step time `CLIMATE_DT_MS`.
  - Map controller output to `fan_stage` (0–3); turn `ac_on` if humidity high or error positive (needs cooling).
  - Clamp outputs; anti-windup (clamp integral).
- **Build steps:**
  1. `inc/app_climate.h` → `void app_climate_step(void);`
  2. `src/app_climate.c` → fixed-point integers only.
  3. Tests: convergence to setpoint; saturation; anti-windup.
- **Key calib:** `CLIMATE_KP`, `CLIMATE_KI`, `CLIMATE_DT_MS`.

## 6) Voice Assist (Stub)
**Purpose:** Parse simple text commands representing voice intents.
- **Inputs:** Non-blocking text line via `hal_read_voice_line()`.
- **Outputs:** Acknowledgement (log) and feature flags (e.g., `sunroof_open` true).
- **Logic:**
  - Check wake phrase `hey car`. Match intents with exact keywords (e.g., `open sunroof`, `set temp 22`).
  - No TTS/ASR—**text only** for PoC.
- **Build steps:**
  1. `inc/app_voice.h` → `void app_voice_step(void);`
  2. `src/app_voice.c` → simple tokenizer + intent table.
  3. Tests: correct intent match; ignore without wake phrase.
- **Key notes:** Keep a small whitelist of intents to stay deterministic.

---

## Scheduler (`src/main.c`)
- Maintain a 10 ms tick using `hal_now_ms()`; on each tick call `*_step()` for all modules in a fixed order.
- Keep each `*_step()` **short** (<< 10 ms). If needed, split long operations across ticks.

---

## Logging & Demo
- `hal_mock_pc.c` reads one row from the scenario CSV per tick and feeds sensors.
- Actuator calls write a **single output row** per tick to `outputs.csv`:
  - Example columns: `ms, brake, wiper_mode, alarm, limit_req, fan_stage, ac_on, blend, park_step`.
- Plot distance/brake, speed/limit/alarm, rain/wiper_mode, temp/fan in any plotting tool.

---

## Unit Tests (Unity)
Create one test file per module (e.g., `tests/test_autobrake.c`). Cover:
- Debounce & hysteresis behavior
- Fault cases (stale/invalid data)
- Boundary conditions just above/below thresholds

Run tests in CI (or locally) and fail build on test/analysis failure.

---

## Hand-off Notes for Claude
- Please scaffold the folders/files exactly as above, with empty “pure logic” modules and a minimal `main.c` loop.
- Implement `hal_mock_pc.c` to read `cfg/scenario_default.csv` and log outputs.
- Add at least one scenario that triggers **each feature**.
- Provide a `CMakeLists.txt` that builds an executable `car_poc` and a `ctest` target for unit tests.
- Include `tools/run_static.sh` to run cppcheck + clang-tidy.
- Keep code MISRA-minded (no dynamic memory, fixed-width types, bounded loops, explicit handling of return codes).
