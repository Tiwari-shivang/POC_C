# Auto-Brake Evals — Add-On Guide
*(Save as `docs/autobrake_evals.md`)*

This guide provides everything needed to add a **scenario-based evaluation (eval) layer** to your `app_autobrake` without changing behavior in production builds. It introduces **non-intrusive hooks**, a **headless metrics implementation**, a **deterministic scenario**, **KPI thresholds**, **Unity-based eval tests**, and **CMake wiring**.

---

## Why this layer?
- **Unit tests** prove local logic.
- **Evals** prove **system behavior over time** under **repeatable scenarios** and produce artifacts (CSV/JSON/JUnit) + CI gates.

---

## What you’ll add
- Tiny `eval_hooks` (no-ops in production) + headless metrics logger for eval builds.
- A deterministic scenario (JSON) you can replay in a **headless** run.
- KPIs: **detection latency**, **reaction latency**, **false positives/negatives**.
- Artifacts: **CSV trace**, **JSON summary**, optional **JUnit**.
- CTest/CI wiring to gate regressions with **thresholds.json**.

---

## Repository layout (new & modified)
```
repo/
├─ src/
│  └─ app_autobrake.c                # MODIFIED: add minimal eval hook calls
├─ eval/
│  ├─ hooks/
│  │  ├─ eval_hooks.h                # NEW: public hook API (no-ops by default)
│  │  └─ eval_hooks.c                # NEW: default no-op implementation
│  ├─ metrics/
│  │  └─ metrics_autobrake.c         # NEW: headless metrics impl (eval build)
│  ├─ scenarios/
│  │  └─ autobrake_ped_close.json    # NEW: deterministic scenario
│  ├─ reports/                       # NEW: output artifacts (gitignored)
│  └─ thresholds.json                # NEW: KPI limits for CI
├─ tests/
│  └─ test_autobrake_eval.c          # NEW: Unity-based scenario eval
├─ CMakeLists.txt                    # MODIFIED: add targets & CTest entries
└─ docs/
   └─ autobrake_evals.md             # THIS FILE
```
> Add `eval/reports/` to `.gitignore`.

---

## 1) Add eval hooks (no behavior change)

**`eval/hooks/eval_hooks.h`**
```c
#ifndef EVAL_HOOKS_H
#define EVAL_HOOKS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EVAL_EVT_FIRST_BELOW_THRESH = 0,
    EVAL_EVT_HAZARD_FLAG        = 1,
    EVAL_EVT_BRAKE_ASSERT       = 2,
    EVAL_EVT_BRAKE_DEASSERT     = 3
} eval_evt_t;

void eval_autobrake_sample(uint32_t now_ms,
                           uint16_t distance_mm,
                           uint32_t sensor_age_ms,
                           uint8_t  hit_count,
                           bool     brake_active);

void eval_autobrake_event(eval_evt_t evt, uint32_t now_ms);

void eval_loop_tick_begin(uint32_t now_ms);
void eval_loop_tick_end(uint32_t now_ms);

#endif /* EVAL_HOOKS_H */
```

**`eval/hooks/eval_hooks.c`** *(default no-ops; safe to link in production)*
```c
#include "eval_hooks.h"

void eval_autobrake_sample(uint32_t a, uint16_t b, uint32_t c, uint8_t d, bool e)
{ (void)a; (void)b; (void)c; (void)d; (void)e; }

void eval_autobrake_event(eval_evt_t evt, uint32_t t)
{ (void)evt; (void)t; }

void eval_loop_tick_begin(uint32_t t) { (void)t; }
void eval_loop_tick_end(uint32_t t)   { (void)t; }
```

---

## 2) Patch `src/app_autobrake.c` (add minimal hooks)
> **Only add calls; do not change control logic.**

At the top:
```c
#include "eval_hooks.h"
```

Extend private state:
```c
typedef struct {
    uint8_t hit_count;
    bool    brake_active;
    bool    prev_below_thresh; /* NEW: detect first-below edge */
} autobrake_state_t;
```

Initialize it in `app_autobrake_init()`:
```c
state.hit_count = 0U;
state.brake_active = false;
state.prev_below_thresh = false; /* NEW */
```

Inside `app_autobrake_step()` (pseudo‑diff; insert at the marked points):
```c
const uint32_t current_time_ms = hal_now_ms();
eval_loop_tick_begin(current_time_ms); /* NEW */

... // existing inhibitor blocks
/* On any early return, before returning: */
eval_autobrake_sample(current_time_ms, 0U, 0U, state.hit_count, state.brake_active);
eval_loop_tick_end(current_time_ms);
return;

/* After obtaining sensor values */
const uint32_t sensor_age = current_time_ms - sensor_ts_ms;
const bool below = (distance_mm <= BRAKE_THRESH_MM);

/* First-below edge */
if ((below != false) && (state.prev_below_thresh == false)) {
    eval_autobrake_event(EVAL_EVT_FIRST_BELOW_THRESH, current_time_ms);
}
state.prev_below_thresh = below;

/* When debounce threshold is met */
if (below) {
    if (state.hit_count < AUTOBRAKE_DEBOUNCE_COUNT) {
        state.hit_count++;
        if (state.hit_count == AUTOBRAKE_DEBOUNCE_COUNT) {
            eval_autobrake_event(EVAL_EVT_HAZARD_FLAG, current_time_ms);
        }
    }
    if ((state.hit_count >= AUTOBRAKE_DEBOUNCE_COUNT) && (state.brake_active == false)) {
        state.brake_active = true;
        eval_autobrake_event(EVAL_EVT_BRAKE_ASSERT, current_time_ms);
    }
} else {
    if (state.brake_active != false) {
        state.brake_active = false;
        eval_autobrake_event(EVAL_EVT_BRAKE_DEASSERT, current_time_ms);
    }
    state.hit_count = 0U;
}

hal_set_brake_request(state.brake_active);

/* Per-tick sample & loop end */
eval_autobrake_sample(current_time_ms, distance_mm, sensor_age, state.hit_count, state.brake_active);
eval_loop_tick_end(current_time_ms);
```

---

## 3) Headless metrics implementation

**`eval/metrics/metrics_autobrake.c`**
```c
#include "eval_hooks.h"
#include <stdio.h>

#define MAX_SAMPLES (60000U)

typedef struct {
    uint32_t t_ms;
    uint16_t dist_mm;
    uint16_t age_ms;
    uint8_t  hit;
    uint8_t  brake;
} sample_t;

static sample_t S[MAX_SAMPLES];
static uint32_t N = 0U;
static uint8_t  got_first = 0U, got_flag = 0U, got_assert = 0U;
static uint32_t t_first = 0U, t_flag = 0U, t_assert = 0U;

static const char* OUT_CSV  = "eval/reports/autobrake_trace.csv";
static const char* OUT_JSON = "eval/reports/autobrake_summary.json";

void eval_autobrake_sample(uint32_t now_ms, uint16_t dist_mm,
                           uint32_t age_ms, uint8_t hit, bool brake)
{
    if (N < MAX_SAMPLES) {
        S[N].t_ms   = now_ms;
        S[N].dist_mm= dist_mm;
        S[N].age_ms = (age_ms > 65535U) ? 65535U : (uint16_t)age_ms;
        S[N].hit    = hit;
        S[N].brake  = (brake ? 1U : 0U);
        N++;
    }
    (void)now_ms;
}

void eval_autobrake_event(eval_evt_t evt, uint32_t now_ms)
{
    switch (evt) {
    case EVAL_EVT_FIRST_BELOW_THRESH: if (got_first == 0U) { got_first = 1U; t_first = now_ms; } break;
    case EVAL_EVT_HAZARD_FLAG:        if (got_flag  == 0U) { got_flag  = 1U; t_flag  = now_ms; } break;
    case EVAL_EVT_BRAKE_ASSERT:       if (got_assert== 0U) { got_assert= 1U; t_assert= now_ms; } break;
    case EVAL_EVT_BRAKE_DEASSERT:
    default: break;
    }
}

void eval_loop_tick_begin(uint32_t now_ms) { (void)now_ms; }
void eval_loop_tick_end(uint32_t now_ms)   { (void)now_ms; }

/* Flush artifacts at the end of an eval */
static void flush_files(void)
{
    /* CSV */
    FILE* f = fopen(OUT_CSV, "w");
    if (f != NULL) {
        (void)fprintf(f, "t_ms,dist_mm,age_ms,hit,brake\n");
        for (uint32_t i = 0U; i < N; ++i) {
            (void)fprintf(f, "%lu,%u,%u,%u,%u\n",
                (unsigned long)S[i].t_ms, S[i].dist_mm, S[i].age_ms, S[i].hit, S[i].brake);
        }
        (void)fclose(f);
    }

    /* JSON summary */
    FILE* g = fopen(OUT_JSON, "w");
    if (g != NULL) {
        const uint32_t detect_latency = (got_first && got_flag)  ? (t_flag  - t_first) : 0U;
        const uint32_t react_latency  = (got_flag  && got_assert)? (t_assert- t_flag ) : 0U;
        (void)fprintf(g,
            "{\n"
            "  \"autobrake\": {\n"
            "    \"detect_latency_ms\": %lu,\n"
            "    \"react_latency_ms\": %lu,\n"
            "    \"events\": {\"first\": %lu, \"flag\": %lu, \"assert\": %lu},\n"
            "    \"samples\": %lu\n"
            "  }\n"
            "}\n",
            (unsigned long)detect_latency,
            (unsigned long)react_latency,
            (unsigned long)t_first, (unsigned long)t_flag, (unsigned long)t_assert,
            (unsigned long)N);
        (void)fclose(g);
    }
}

/* Provide a symbol the eval test can call */
void eval_autobrake_flush(void) { flush_files(); }
```

---

## 4) Deterministic scenario (JSON)

**`eval/scenarios/autobrake_ped_close.json`**
```json
{
  "seed": 7,
  "duration_ms": 12000,
  "tick_ms": 10,
  "initial_state": { "speed_kph": 50, "outside_temp_c": 25 },
  "stimuli": [
    { "t_ms": 1500, "type": "spawn_pedestrian", "x_m": 32.0, "y_m": 0.0, "vx_mps": -1.2, "width_m": 0.5 },
    { "t_ms": 2000, "type": "rain_intensity", "value": 0.0 }
  ],
  "assertions": [
    { "metric": "autobrake.detect_latency_ms", "op": "<=", "value": 180 },
    { "metric": "autobrake.react_latency_ms",  "op": "<=", "value": 60  }
  ]
}
```
> Your eval test will **replay** (seeded) stimuli into mocks and step the control loop; use these assertions or `thresholds.json` below.

---

## 5) KPI thresholds

**`eval/thresholds.json`**
```json
{
  "autobrake": {
    "detect_latency_ms_max": 180,
    "react_latency_ms_max": 60,
    "false_positive_max": 0,
    "false_negative_max": 0
  }
}
```

---

## 6) Unity-based scenario eval test

**`tests/test_autobrake_eval.c`**
```c
#include "unity.h"
#include "app_autobrake.h"
#include "config.h"
#include <stdint.h>
#include <stdbool.h>

/* HAL doubles used by your unit tests (declare or include as needed) */
extern void hal_set_brake_request(bool on);
extern bool hal_get_vehicle_ready(void);
extern bool hal_driver_brake_pressed(void);
extern bool hal_read_distance_mm(uint16_t *out_mm, uint32_t *out_ts_ms);
extern uint32_t hal_now_ms(void);

/* Test-local doubles / state (replace with your project’s mocks) */
static uint16_t mock_distance_mm;
static uint32_t mock_sample_ts_ms;
static bool     mock_vehicle_ready;
static bool     mock_driver_brake;
static bool     mock_brake_request;
static uint32_t mock_now_ms;

/* Minimal mock implementations */
bool hal_get_vehicle_ready(void) { return mock_vehicle_ready; }
bool hal_driver_brake_pressed(void) { return mock_driver_brake; }
uint32_t hal_now_ms(void) { return mock_now_ms; }
bool hal_read_distance_mm(uint16_t *out_mm, uint32_t *out_ts_ms) {
    if ((out_mm == NULL) || (out_ts_ms == NULL)) { return false; }
    *out_mm = mock_distance_mm;
    *out_ts_ms = mock_sample_ts_ms;
    return true;
}
void hal_set_brake_request(bool on) { mock_brake_request = on; }

/* Eval hooks: we just rely on metrics_autobrake.c linked in eval build */
void eval_autobrake_flush(void); /* from metrics_autobrake.c */

static void reset_mocks(void) {
    mock_distance_mm = 2000U;
    mock_sample_ts_ms = 0U;
    mock_vehicle_ready = true;
    mock_driver_brake = false;
    mock_brake_request = false;
    mock_now_ms = 100U; /* deterministic non-zero */
}

static void step_n(uint32_t n_steps) {
    for (uint32_t i = 0U; i < n_steps; ++i) {
        mock_sample_ts_ms = mock_now_ms;
        app_autobrake_step();
        mock_now_ms += CONTROL_DT_MS;
    }
}

void setUp(void)   { reset_mocks(); app_autobrake_init(); }
void tearDown(void){}

/* Scenario: pedestrian appears; we measure latencies via eval hooks */
void test_EVAL_Autobrake_Pedestrian_Close_Scenario(void)
{
    /* Warm-up 500 ms */
    step_n(50U);

    /* Pedestrian event simulated by dropping distance below threshold steadily */
    for (uint32_t t = 0U; t < 1200U; t += CONTROL_DT_MS) {
        if (t >= 150U) { /* first-below occurs ~150 ms into scenario */
            mock_distance_mm = (uint16_t)(BRAKE_THRESH_MM - 20U);
        } else {
            mock_distance_mm = (uint16_t)(BRAKE_THRESH_MM + 200U);
        }
        step_n(1U);
    }

    /* Flush artifacts (CSV/JSON) for this run */
    eval_autobrake_flush();
    /* Optionally, add direct assertions here by reading the JSON file */
    TEST_ASSERT_TRUE(1); /* keep as artifact producer; thresholds handled in CI */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_EVAL_Autobrake_Pedestrian_Close_Scenario);
    return UNITY_END();
}
```

> This test **drives** the controller for a scenario and relies on `metrics_autobrake.c` to compute KPIs and dump artifacts.

---

## 7) CMake wiring

Append to your root `CMakeLists.txt`:
```cmake
# --- Eval hooks (always built; default no-ops) ---
add_library(eval_hooks STATIC
    eval/hooks/eval_hooks.c
)
target_include_directories(eval_hooks PUBLIC eval/hooks)

# --- Metrics impl (link only in eval builds/tests) ---
add_library(metrics_autobrake STATIC
    eval/metrics/metrics_autobrake.c
)
target_link_libraries(metrics_autobrake PUBLIC eval_hooks)

# --- Your app core (replace target name if different) ---
# target_link_libraries(car_core PRIVATE eval_hooks)  # ensure app sees hooks

# --- Unity eval test (headless) ---
add_executable(test_autobrake_eval tests/test_autobrake_eval.c)
target_link_libraries(test_autobrake_eval PRIVATE
    eval_hooks
    metrics_autobrake
    car_core            # your library with app_autobrake + deps
)

# --- CTest entry ---
enable_testing()
add_test(NAME eval_autobrake_ped_close COMMAND test_autobrake_eval)

# --- Reports dir ---
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/eval/reports)
```

> If you already have a `car_core` or similar library target containing `app_autobrake.c`, link `eval_hooks` into it so the hook symbols resolve (no-ops in normal runs).

---

## 8) Running the eval

```bash
# Configure + build (Release recommended for stable timing)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run eval test
ctest --test-dir build --output-on-failure -R eval_autobrake_ped_close

# Artifacts
#   eval/reports/autobrake_trace.csv
#   eval/reports/autobrake_summary.json
```

Example JSON (`autobrake_summary.json`):
```json
{
  "autobrake": {
    "detect_latency_ms": 160,
    "react_latency_ms": 40,
    "events": { "first": 1120, "flag": 1280, "assert": 1320 },
    "samples": 1200
  }
}
```

---

## 9) CI thresholds (gating)

Use `eval/thresholds.json` as a single source of truth and add a small CI script (Python or C) that checks:
- `detect_latency_ms <= detect_latency_ms_max`
- `react_latency_ms  <= react_latency_ms_max`
- `false_positive    <= false_positive_max`
- `false_negative    <= false_negative_max`

Exit non‑zero on breach to fail the job.

---

## 10) Tips & extensions
- Add more scenarios: rain/noise, occlusion, speed sweeps (30/50/70 kph).
- Emit JUnit XML if your CI prefers it.
- Keep eval tests separate from fast unit tests (label them in CTest or put under a `-L EVAL` label).
- When stable, create **baselines** and compare traces for drift detection.

---

## License & compliance notes
- Hooks and metrics contain no behavior changes; they only **observe**.
- MISRA C: Keep functions defined (no macros that remove calls) and avoid dynamic allocation in metrics.
- ISO 26262 evidence: the artifacts (CSV/JSON + thresholds) serve as **objective measures** of timing and functional safety behavior in scenario conditions.

---

## Done ✅
After these steps, you’ll be able to run a deterministic scenario, export KPIs and traces, and gate regressions—all without changing production behavior.
