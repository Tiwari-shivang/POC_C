# Car PoC — Laptop‑Only Embedded C Application (Requirements & Build Guide)

**Purpose:** Specify everything needed for Cursor (or any dev) to create a **single C application** that simulates multiple in‑vehicle features **without hardware**, with **MISRA‑minded** code structure, **SDL2** interactive UI option, and **CSV‑driven headless** mode.

---

## 0) Scope & Goals

- **Runs on a laptop only** (no MCU). Two build modes:
  - **Headless:** replay inputs from a Scenario CSV; log outputs to CSV.
  - **Interactive (SDL2):** render a simple dashboard; inject inputs via keyboard / text.
- **Feature modules (pure logic):**
  1. Auto Braking
  2. Rain‑Sensing Wipers
  3. Speed Governor + Traffic Sign Events
  4. Auto Parking (simplified guidance)
  5. Auto Climate Control
  6. Voice Assist (text‑stub: “hey car …”)
- **Architecture:** Embedded‑style layering (HAL + app modules); **no dynamic memory**, fixed‑width types, deterministic behavior.
- **Deliverables:** Buildable C project (CMake), unit tests (Unity), static analysis scripts (cppcheck, clang‑tidy), optional SDL2 UI.

**Non‑Goals:** Actual CV/sensor drivers; precise vehicle dynamics; networking/UDS; AUTOSAR compliance; production MISRA certification (we follow MISRA **guidelines** in spirit).

---

## 1) Project Creation Guidelines

### 1.1 Toolchain & Dependencies (PC)
- **Compiler:** GCC or Clang (C99)
- **Build:** CMake ≥ 3.16
- **Unit tests:** Unity (optionally via Ceedling)
- **Static analysis:** Cppcheck, clang‑tidy (CERT profile)
- **Formatting:** clang‑format
- **Optional UI:** SDL2 (video + timer; audio optional)

**Compiler flags (baseline):**
```
-std=c99 -O0 -g3 -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wformat=2 -Wundef
```

**Static analysis (suggested commands):**
```
cppcheck --enable=warning,performance,portability --std=c99 --inline-suppr src inc
clang-tidy src/*.c -- -std=c99
```

### 1.2 Project Layout
```
car_poc/
├─ CMakeLists.txt
├─ README.md
├─ docs/
│  ├─ requirements.md         # copy of THIS doc or summary
│  ├─ misra_policy.md         # rules followed & deviations
│  └─ test_plan.md
├─ cfg/
│  ├─ calib.h                 # thresholds & gains (integers only)
│  └─ scenario_default.csv    # time-series demo inputs
├─ inc/
│  ├─ platform.h              # timebase, assert, compile-time config
│  ├─ hal.h                   # abstract sensor/actuator interface
│  ├─ app_autobrake.h
│  ├─ app_wipers.h
│  ├─ app_speedgov.h
│  ├─ app_autopark.h
│  ├─ app_climate.h
│  └─ app_voice.h
├─ src/
│  ├─ main.c                  # 10 ms scheduler
│  ├─ platform_pc.c           # headless time/log helpers
│  ├─ hal_mock_pc.c           # CSV replayer + output logger
│  ├─ platform_sdl.c          # SDL init, event pump, renderer (optional)
│  ├─ hal_sdl.c               # SDL-based HAL (keyboard in, UI out) (optional)
│  ├─ io_logger.c             # outputs.csv writer
│  ├─ app_autobrake.c         # PURE logic (no I/O)
│  ├─ app_wipers.c
│  ├─ app_speedgov.c
│  ├─ app_autopark.c
│  ├─ app_climate.c
│  └─ app_voice.c
├─ sim/
│  ├─ scenario.h              # CSV schema + parser
│  ├─ scenario.c
│  └─ scenarios/              # additional demo drives
│     ├─ city_50kph.csv
│     ├─ highway_100kph.csv
│     └─ rain_parking.csv
├─ tests/
│  ├─ unity/                  # vendored Unity
│  ├─ test_autobrake.c
│  ├─ test_wipers.c
│  ├─ test_speedgov.c
│  ├─ test_autopark.c
│  └─ test_climate.c
└─ tools/
   ├─ run_static.sh           # cppcheck + clang-tidy
   └─ format.sh               # clang-format
```

### 1.3 Build Modes
- **HEADLESS (default or via `-DHEADLESS=ON`):**
  - Compile `hal_mock_pc.c` + `platform_pc.c`
  - `./car_poc --scenario cfg/scenario_default.csv`
  - Logs **outputs.csv** for plotting
- **SDL2 (interactive) (`-DHEADLESS=OFF`):**
  - Compile `hal_sdl.c` + `platform_sdl.c`
  - Keyboard controls & on‑screen dashboard

**CMake Options (example):**
```cmake
option(HEADLESS "Build without SDL2 (CSV replayer)" ON)
if(NOT HEADLESS)
  find_package(SDL2 REQUIRED)
  target_link_libraries(car_poc PRIVATE ${SDL2_LIBRARIES})
  target_include_directories(car_poc PRIVATE ${SDL2_INCLUDE_DIRS})
endif()
```

---

## 2) MISRA‑Minded Coding Guidelines

> We **follow MISRA C principles** (not reproducing proprietary rule text). Enforce via design + static checks + a **deviations log**.

- **C dialect:** C99
- **Types:** use `stdint.h` fixed‑width types exclusively (`uint16_t`, `int32_t`, …). No plain `int` unless deliberate and documented.
- **Memory:** **no dynamic allocation** (`malloc/free`) and **no recursion**.
- **Initialization:** initialize all variables; no use of uninitialized data.
- **Bounds:** bounded loops/counters; check array indices; avoid pointer arithmetic unless essential & reviewed.
- **Control flow:** single‑purpose, short functions; avoid deep nesting; deterministic state machines.
- **Casts:** explicit and justified casts; avoid implicit narrowing.
- **Headers:** no non‑reentrant global state; `static` internal linkage for private functions.
- **Side effects:** avoid multiple side effects in expressions; no hidden macros with side effects.
- **Volatile/ISR:** not used in PoC; if added, isolate & document.
- **Error handling:** check **all** return values; fail‑safe defaults.
- **Deviations:** record in `docs/misra_policy.md` with: Rule ID/Intent, Location, Rationale, Risk, Mitigation.

**Automation:** treat warnings as errors; run cppcheck + clang‑tidy on every build (`tools/run_static.sh`).

---

## 3) Common Interfaces (HAL & Platform)

### 3.1 `inc/hal.h`
```c
#ifndef HAL_H
#define HAL_H
#include <stdint.h>
#include <stdbool.h>

bool     hal_get_vehicle_ready(void);
bool     hal_driver_brake_pressed(void);
uint32_t hal_now_ms(void);

/* Sensors / perception outputs (stubbed) */
bool hal_read_distance_mm(uint16_t* out_mm, uint32_t* out_ts_ms);
bool hal_read_rain_level_pct(uint8_t* out_pct, uint32_t* out_ts_ms);
bool hal_read_vehicle_speed_kph(uint16_t* out_kph, uint32_t* out_ts_ms);
bool hal_poll_speed_limit_kph(uint16_t* out_limit_kph); /* event once */
typedef struct { bool found; uint16_t width_mm; } park_gap_t;
bool hal_parking_gap_read(park_gap_t* out, uint32_t* out_ts_ms);
bool hal_read_cabin_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms);
bool hal_read_ambient_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms);
bool hal_read_humidity_pct(uint8_t* out_pct, uint32_t* out_ts_ms);
bool hal_read_voice_line(char* buf, uint16_t len);       /* non-blocking */

/* Actuators (logged / drawn) */
void hal_set_brake_request(bool on);
void hal_set_wiper_mode(uint8_t mode);   /* 0=OFF,1=INT,2=LOW,3=HIGH */
void hal_set_alarm(bool on);
void hal_set_speed_limit_request(uint16_t kph);
void hal_set_climate(uint8_t fan_stage, bool ac_on, uint8_t blend_pct);
void hal_actuate_parking_prompt(uint8_t step_code);
#endif
```

### 3.2 `inc/platform.h`
```c
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>
#define TICK_MS (10U)
void platform_assert(bool cond);
#endif
```

### 3.3 `cfg/calib.h` (integers only)
```c
/* Time */
#define SENSOR_STALE_MS            (100U)

/* Auto Braking */
#define AB_THRESHOLD_MM            (1220U)  /* ≈ 4 ft */
#define AB_DEBOUNCE_HITS          (3U)

/* Wipers */
#define WIPER_T_RAIN_INT          (20U)    /* % */
#define WIPER_T_RAIN_LOW          (45U)
#define WIPER_T_RAIN_HIGH         (75U)

/* Speed Governor */
#define SPEED_ALARM_TOL_KPH       (3U)
#define SPEED_ALARM_DEBOUNCE      (2U)
#define SPEED_HYSTERESIS_KPH      (5U)

/* Climate */
#define CLIMATE_KP                (8)
#define CLIMATE_KI                (1)
#define CLIMATE_DT_MS             (1000U)

/* Parking (simplified) */
#define PARK_MIN_GAP_MM           (5000U)
#define PARK_SCAN_LEN_SAMPLES     (50U)
```

### 3.4 Scenario CSV Schema
Columns (typical):  
`ms, distance_mm, rain_pct, speed_kph, sign_event, gap_found, gap_width_mm, cabin_tc_x10, ambient_tc_x10, humid_pct, setpoint_x10, voice_cmd`

- **`ms`**: row timestamp; **monotonic**
- **`sign_event`**: 0 = none; else limit value (30/50/80/100…)
- **`gap_found`**: 0/1; **`gap_width_mm`** if found
- **`voice_cmd`**: text (optional)

### 3.5 Logging (outputs.csv)
Columns: `ms, brake, wiper_mode, alarm, limit_req, fan_stage, ac_on, blend, park_step`

---

## 4) SDL2 Integration

**Goal:** Provide a visual HUD and keyboard‑driven inputs. SDL2 stays in the **HAL/Platform**; app modules do not include SDL.

### 4.1 Files
- `src/platform_sdl.c`: init/quit, `hal_now_ms()` via `SDL_GetTicks()`, `platform_sdl_pump_events()`, `platform_sdl_sleep()`
- `src/hal_sdl.c`: maintain simulated sensor state; handle key bindings; draw HUD each tick; provide actuator sinks.

### 4.2 Key Bindings (suggestion)
- **Speed:** `↑/↓` ±1 kph
- **Distance:** `←/→` ±50 mm
- **Rain:** `r/R` ±1%
- **Speed limit events:** `1/2/3/4` → 30/50/80/100 kph (posted once)
- **Parking gap:** `P` toggles found (width default 5500 mm)
- **Driver brake override:** `b` toggle
- **Climate setpoint:** `t/g` +0.5/−0.5°C (x10 fixed‑point)
- **Voice input:** `/` open mini line; type `hey car open sunroof`
- **Quit:** `Esc` or window close

### 4.3 HUD Elements
- Numeric speed + current speed limit bubble
- Obstacle distance bar (turns red under threshold) + BRAKE lamp
- Wiper mode text (OFF/INT/LOW/HIGH)
- Alarm indicator
- Climate: setpoint, cabin temp, fan stage, AC on/off
- Parking: current step prompt
- Voice: last parsed intent

### 4.4 CMake
Provide `HEADLESS` option. If off, link SDL2 and compile SDL files.

---

## 5) Stubs & Replacements (No Hardware)

| Real Device/Source | Stub/Replacement | HAL Function(s) | Notes |
|---|---|---|---|
| Traffic sign camera (CV) | CSV `sign_event`, or keys `1/2/3/4` | `hal_poll_speed_limit_kph` | Event returns **true once** when posted. |
| Parking camera (gap) | CSV `gap_found/width`, or key `P` | `hal_parking_gap_read` | Minimal geometry; boolean + width‑mm. |
| Rain sensor | CSV `rain_pct`, or keys `r/R` | `hal_read_rain_level_pct` | Add small jitter for hysteresis testing. |
| Front distance sensor | CSV `distance_mm`, or arrows `←/→` | `hal_read_distance_mm` | Script approach profile for demo. |
| Vehicle speed | CSV `speed_kph`, or keys `↑/↓` | `hal_read_vehicle_speed_kph` | Debounce/hyst. handled in feature logic. |
| Voice ASR/NLU | Text line `/` → string | `hal_read_voice_line` | Must start with “hey car”. |

---

## 6) Feature Requirements & How to Create

> Each feature is a **pure module** (`src/app_*.c`). All I/O goes via HAL. Keep functions small, deterministic; test with Unity.

### 6.1 Auto Braking
**Objective:** Request brake when obstacle ≤ threshold for N ticks.  
**Inputs:** `distance_mm` (+ ts), `hal_get_vehicle_ready()`, `hal_driver_brake_pressed()`  
**Outputs:** `hal_set_brake_request(bool)`  
**Algorithm:**  
- Validate freshness: `(now - ts) <= SENSOR_STALE_MS`  
- Inhibit if vehicle not ready or driver pressed  
- Debounce: `hits++` while `distance_mm <= AB_THRESHOLD_MM`; else `hits=0`  
- Assert brake when `hits >= AB_DEBOUNCE_HITS`, clear otherwise  
**Calib:** `AB_THRESHOLD_MM`, `AB_DEBOUNCE_HITS`  
**Unit tests:** threshold edges, stale data, override behavior

### 6.2 Rain‑Sensing Wipers
**Objective:** Map rain% to discrete modes with hysteresis.  
**Inputs:** `rain_pct` (+ ts), optional speed bias  
**Outputs:** `hal_set_wiper_mode(0..3)`  
**Algorithm:** threshold banding with hysteresis around `INT/LOW/HIGH` boundaries; cap to max at very high rain%  
**Calib:** `WIPER_T_RAIN_*`  
**Unit tests:** transition up/down with noise; no chatter near thresholds

### 6.3 Speed Governor + Traffic Sign Events
**Objective:** Alarm if speed exceeds current limit; optional limit request.  
**Inputs:** `speed_kph`, `hal_poll_speed_limit_kph()`  
**Outputs:** `hal_set_alarm(bool)`, `hal_set_speed_limit_request(kph)` (optional)  
**Algorithm:**  
- Update `current_limit` on event  
- Over‑speed latch if `speed > limit + tol` for `N` ticks  
- Clear when `speed < limit − hysteresis`  
**Calib:** `SPEED_ALARM_TOL_KPH`, `SPEED_ALARM_DEBOUNCE`, `SPEED_HYSTERESIS_KPH`  
**Unit tests:** event timing, latch/clear, boundary speeds

### 6.4 Auto Parking (Simplified)
**Objective:** Detect gap and issue fixed guidance steps.  
**Inputs:** `park_gap_t` (found + width) at low vehicle speed  
**Outputs:** `hal_actuate_parking_prompt(step_code)`  
**Algorithm:**  
- **SCAN**: moving window over recent gap samples; if `found && width>=PARK_MIN_GAP_MM` → sequence  
- **SEQUENCE** (scripted): REVERSE_RIGHT (n ticks) → STRAIGHTEN (m) → REVERSE_LEFT (n) → DONE  
- Abort on excessive speed or timeout  
**Calib:** `PARK_MIN_GAP_MM`, `PARK_SCAN_LEN_SAMPLES`  
**Unit tests:** gap/no‑gap, timeouts, step ordering

### 6.5 Auto Climate Control
**Objective:** Track setpoint with discrete PI; stage fan; toggle AC for cooling/humidity.  
**Inputs:** `cabin_temp_x10`, `ambient_temp_x10`, `humidity_pct`, `user_setpoint_x10`  
**Outputs:** `hal_set_climate(fan_stage, ac_on, blend_pct)`  
**Algorithm:** integer PI with `CLIMATE_KP/KI`, sample time `CLIMATE_DT_MS`; clamp and anti‑windup; map controller output to fan stage 0..3; `ac_on` when cooling required or high humidity  
**Unit tests:** convergence from hot/cold starts; saturation; anti‑windup

### 6.6 Voice Assist (Text Stub)
**Objective:** Parse simple text commands gated by wake phrase.  
**Inputs:** `hal_read_voice_line(buf,len)`  
**Outputs:** actions/flags (e.g., print “Opening sunroof”).  
**Algorithm:** check prefix `hey car`; tokenize and match intents (e.g., `open sunroof`, `set temp 22`)  
**Unit tests:** accept valid commands; reject without wake phrase; unknown intents

---

## 7) Scheduler & Timing

- **Period:** 10 ms (`TICK_MS`)  
- **Main loop:** read time via `hal_now_ms()`, call each `*_step()` once per tick in fixed order  
- Keep each step **≪ 10 ms**; avoid blocking (no file I/O, no sleeps inside steps).

```c
static void tick_10ms(void) {
  app_autobrake_step();
  app_wipers_step();
  app_speedgov_step();
  app_autopark_step();
  app_climate_step();
  app_voice_step();
}
```

---

## 8) Build, Run, Test

### 8.1 Build
```bash
mkdir build && cd build
cmake -DHEADLESS=ON ..     # use OFF for SDL2 interactive build
make -j
```

### 8.2 Run (Headless)
```bash
./car_poc --scenario ./cfg/scenario_default.csv
# outputs to ./outputs.csv
```

### 8.3 Run (SDL2)
```bash
./car_poc
# drive with keyboard; HUD shows sensors & outputs
```

### 8.4 Static Checks & Tests
```bash
./tools/run_static.sh      # cppcheck + clang-tidy; fail on warnings
ctest                     # if tests added to CTest, or
# ceedling test           # if using Ceedling
```

---

## 9) Implementation Notes

- **Pure modules:** `app_*.c` must not include SDL or stdio; only `hal.h` and `cfg/calib.h` (and basic headers).
- **Integer math:** use fixed‑point (`_x10`) for temperatures etc.
- **Logs/UI:** only in HAL/platform; app modules should be side‑effect free except via HAL outputs.
- **Deviations:** if a MISRA rule must be deviated (e.g., for a macro), document it in `docs/misra_policy.md`.

---

## 10) Acceptance Checklist

- [ ] Builds headless & SDL2 modes
- [ ] Scenario CSV plays and logs outputs.csv
- [ ] SDL2 HUD renders; keys inject inputs
- [ ] All six features implemented and unit‑tested
- [ ] Static analysis passes; formatting applied
- [ ] MISRA deviations file present and justified

---

## Appendix A — Minimal CLI
- `--scenario <path>`: headless input CSV
- `--rt-slow <factor>`: slow down playback for demos (optional)
- `--out <path>`: outputs CSV path

## Appendix B — Suggested Test Cases
- Debounce thresholds exactly at edges
- Hysteresis around limits (speed, rain)
- Stale sensor timestamps
- Parking gap exactly at min width
- Climate setpoint steps up/down; ambient step change
- Voice command with/without wake phrase
